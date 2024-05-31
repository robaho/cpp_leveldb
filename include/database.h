#pragma once

#include <memory>
#include <string>
#include <atomic>

#include "segment.h"
#include "memorysegment.h"
#include "waitgroup.h"
#include "options.h"
#include "threadpool.h"
#include "lockfile.h"
#include "writebatch.h"
#include "deleter.h"

class Merger;

class DatabaseState {
friend class Database;
friend class Merger;
private:
    const std::vector<SegmentRef> segments;
    const MemorySegmentRef memory;
    const SegmentRef multi;
public:
    DatabaseState(const std::vector<SegmentRef>& segments, const MemorySegmentRef& memory, const SegmentRef& multi)
        : segments(segments), memory(memory), multi(multi) {}
    DatabaseState(const DatabaseState& other) : segments(other.segments), memory(other.memory), multi(other.multi){}
};

class Database;

typedef std::shared_ptr<Database> DatabaseRef;

#include "merger.h"

class Snapshot;

typedef std::shared_ptr<Snapshot> SnapshotRef;

class Database {
friend class Merger;

private:
    static constexpr int dbMemorySegment = 1024 * 1024;
    static constexpr int dbMaxSegments = 8;

    static std::mutex global_lock;
    
    std::recursive_mutex db_lock;

    static ThreadPool executor;

    bool _open;
    std::atomic<uint64_t> nextSegID{0};
    std::atomic<bool> closing;
    std::atomic<std::exception*> err;
    LockFile lockFile;
    WaitGroup wg;
    const Options options;
    std::exception_ptr error; // if non-null and async error has occurred
    std::shared_ptr<const DatabaseState> state;
    std::weak_ptr<Database> weakRef;
    Merger merger;
    Deleter deleter;

public:
    /**
     * @brief open a database. a database can only be opened exclusively by a single process.
     * 
     * @param path the path of the database (the directory name)
     * @param options the database options
     * @return a reference to the Database
     * @throws DatabaseInUse if the database is already open
     * @throws DatabaseNotFound if the database does not exist
     * @see DatabaseException
     */
    static DatabaseRef open(const std::string& path, const Options& options);
    /**
     * @brief remove a database from the system
     * 
     * @param path 
     * @throws DatabaseInUse if the database is in use
     * @throws DatabaseNotFound if the database does not exist
     */
    static void remove(const std::string& path);
    /**
     * @brief the path of the database
     */
    const std::string path;
    /**
     * @brief get the value associated with the key
     * 
     * @param key the key
     * @return ByteBuffer of the value which will be empty if the key is not found 
     */
    ByteBuffer get(const Slice& key);
    /**
     * @brief get the value associated with the key storing into the provided ByteBuffer
     * 
     * @param key the key
     * @param value the ByteBuffer to store the value in
     * @return a reference to the provided ByteBuffer. The value will be empty if the key is not found 
     */
    ByteBuffer& get(const Slice& key,ByteBuffer& value);
    /**
     * @brief put a key/value pair into the database
     * 
     * @param key must be non-empty
     * @param value must be non-empty
     */
    void put(const Slice& key,const Slice& value);
    /**
     * @brief remove a key/value pair from the database
     * 
     * @param key the key to remove
     * @return the value that was removed, or empty if the key did not exist
     */
    ByteBuffer remove(const Slice& key);
    /**
     * @brief efficiently write multiple key/value pairs into the database
     * 
     * @param batch the batch
     */
    void write(const WriteBatch& batch);
    /**
     * @brief get an Iterator for the Database. This is a backed by a Snapshot, so the iterator
     will not reflect pairs added or removed after the lookup is returned.
     * 
     * @param lower the lower range, or empty
     * @param upper the upper range, or empty
     * @return a shared reference to the iterator
     */
    LookupRef lookup(const Slice& lower,const Slice& upper);
    /**
     * @brief get a read-only Snapshot of the database, which is constant after created
     * 
     * @return a reference to the Snapshot
     */
    SnapshotRef snapshot();
    /**
     * @brief close the database, compacting to the maximum number of segments configured when the
     database was opened.
     */
    void close() { closeWithMerge(options.maxSegments); }
    /**
     * @brief close the database specifying the maximum number of segments
     * 
     * @param mergeCount if 0, then the database is closed without any compaction 
     */
    void closeWithMerge(int mergeCount);
private:
    static void checkValidDatabase(const std::string& path);
    static DatabaseRef create(const std::string& path, const Options& options);
    static DatabaseRef openImpl(const std::string& path, const Options& options);
    Database(const std::string& path,const LockFile& lockFile,const Options& options);
    uint64_t nextSegmentID() { return ++nextSegID; }
    void maybeSwapMemory();
    void maybeMerge();
    std::shared_ptr<const DatabaseState> getState() {
        return std::atomic_load(&state);
    }
    void setState(const DatabaseState &newState) {
        std::atomic_store(&state,std::make_shared<const DatabaseState>(newState));
    }
};

#define DB_LOCK() std::lock_guard<std::recursive_mutex> lock(db_lock)
#define DB_UNLOCK() db_lock.unlock()

class Snapshot {
friend class Database;
private:
    DatabaseRef db;
    SegmentRef multi;
    Snapshot(DatabaseRef db, SegmentRef multi) : db(db), multi(multi){}
public:
    /**
     * @brief get the value associated with the key
     * 
     * @param key the key
     * @return ByteBuffer of the value which will be empty if the key is not found 
     */
    ByteBuffer get(const Slice& key);
    /**
     * @brief get the value associated with the key storing into the provided ByteBuffer
     * 
     * @param key the key
     * @param value the ByteBuffer to store the value in
     * @return a reference to the provided ByteBuffer. The value will be empty if the key is not found 
     */
    ByteBuffer& get(const Slice& key,ByteBuffer& value);
    /**
     * @brief get an Iterator for the Snapshot
     * 
     * @param lower the lower range, or empty
     * @param upper the upper range, or empty
     * @return a shared reference to the iterator
     */
    LookupRef lookup(const Slice& lower, const Slice& upper);
};

#define checkKey(x) if(x.empty()) throw EmptyKey(); if(x.length>1024) throw KeyTooLong();
