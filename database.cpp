#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <exception>
#include <regex>

#include "database.h"
#include "exceptions.h"
#include "disksegment.h"
#include "memorysegment.h"
#include "multisegment.h"
#include "merger.h"

namespace fs = std::filesystem;

std::mutex Database::global_lock;

ThreadPool Database::executor;

DatabaseRef Database::open(const std::string& path, const Options& options) {
    std::lock_guard<std::mutex> lock(global_lock);
    try {
        return openImpl(path, options);
    } catch (const DatabaseNotFound& e) {
        if (options.createIfNeeded)
            return create(path, options);
        throw e;
    }
}

DatabaseRef Database::create(const std::string& path, const Options& options) {
    if (!std::filesystem::create_directories(path)) {
        throw DatabaseException("unable to create directories");
    }

    return openImpl(path, options);
}

DatabaseRef Database::openImpl(const std::string& path, const Options& in_options) {
    checkValidDatabase(path);

    std::string lockFilePath = (std::filesystem::path(path) / "lockfile").string();
    LockFile lockFile(lockFilePath);
    if (!lockFile.tryLock())
        throw DatabaseInUse();

    Options options(in_options);
    if(options.maxMemoryBytes < dbMemorySegment) {
        options.maxMemoryBytes = dbMemorySegment;
    }
    if(options.maxSegments < dbMaxSegments) {
        options.maxSegments = dbMaxSegments;
    }

    auto db = new Database(path,lockFile,options);

    db->deleter.deleteScheduled();

    auto segments = DiskSegment::loadDiskSegments(path,options);
    uint64_t maxSegID = 0;
    for(auto s : segments) {
        maxSegID = MAX(maxSegID,s->upperID());
    }
    db->nextSegID = maxSegID;

    auto ms = MemorySegment::newMemorySegment(db->path,db->nextSegmentID(),db->options);
    auto multis = MultiSegment::newMultiSegment(copyAndAppend(segments,ms));

    db->setState(DatabaseState(segments,ms,multis));

    DatabaseRef ref(db);
    db->weakRef = ref;

    if(!options.disableAutoMerge) {
        db->wg.add(1);
        db->merger.autoMerger(db);
    }

    return ref;
}

Database::Database(const std::string& path,const LockFile& lockFile,const Options& options)
     : _open(true) , lockFile(lockFile) , options(options), deleter(path), path(path) {
}

void Database::checkValidDatabase(const std::string& path) {
    if(!fs::exists(path)) throw DatabaseNotFound();
    
    if(!fs::is_directory(path)) throw InvalidDatabase();

    std::cmatch m;
    std::regex re("(log|keys|data)\\..*");

    for(auto file : fs::directory_iterator(path)) {
        auto name = file.path().filename().string();
        if(name=="lockfile") continue;
        if(name=="deleted") continue;
        if(!std::regex_match(name.c_str(),m,re)) throw InvalidDatabase();
    }
}

void Database::remove(const std::string& path) {
    std::lock_guard<std::mutex> lock(global_lock);

    checkValidDatabase(path);
    std::string lockFilePath = (std::filesystem::path(path) / "lockfile").string();
    LockFile lockfile(lockFilePath);
    if(!lockfile.tryLock()) {
        throw DatabaseInUse();
    }
    fs::remove_all(path);
}

void Database::closeWithMerge(int segmentCount) {
    std::unique_lock global(global_lock);

    if(!_open) throw DatabaseClosed();

    closing=true;
    merger.wakeup();

    // wait for background merger to finish
    wg.waitEmpty();

    DB_LOCK();

    auto state = getState();
    DatabaseState newState(copyAndAppend(state->segments,state->memory),MemorySegmentRef(0),MultiSegmentRef(0));
    setState(newState);
    if(segmentCount>0) {
        merger.mergeSegments0(this,segmentCount,false);
    }
    state = getState();
    for(auto s : state->segments) {
        auto seg = s.get();
        if(typeid(*seg)!=typeid(MemorySegment)) continue;
        WaitGroup *wgptr = &wg;
        wgptr->add(1);
        executor.enqueue([seg,this,wgptr]() {
            writeSegmentToDisk(this,seg);
            wgptr->done();
        });
    }
    wg.waitEmpty();
    auto segments = getState()->segments;
    for(auto s : segments) {
        s->close();
    }
    _open=false;
    deleter.deleteScheduled();
}

