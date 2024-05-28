#pragma once

#include <string>
#include <filesystem>

#include "keyvalue.h"
#include "skiplist.h"
#include "skiplistitr.h"
#include "options.h"
#include "logfile.h"
#include "segment.h"

namespace fs = std::filesystem;

class LogSegment final : public Segment {
private:
    uint64_t id;
    SkipList<const KeyValue> list;
    std::string path;
    Options options;
    uint64_t filesize;
    LogSegment(std::string path,Options options) : list(keyValueCompare(options)), path(path), options(options) {
    }
public:
    static SegmentRef newLogSegment(const std::string& path,const Options& options) {
        auto ls = new LogSegment(path,options);
        readLogFile(ls->list,path,options);
        auto ref =SegmentRef(ls);
        ref->weakRef = ref;
        return ref;
    }
    uint64_t size() override { return filesize; }
    ID lowerID() override { return id; }
    ID upperID() override { return id; }
    ByteBuffer& get(const Slice& key,ByteBuffer& value) override {
        value = list.get(Key(key)).value;
        return value;
    }
    ByteBuffer put(const Slice& key,const Slice& value) override {
        throw IllegalState("put() called on LogSegment");
    }
    ByteBuffer remove(const Slice& key) override {
        throw IllegalState("remove() called on LogSegment");
    }
    LookupRef lookup(const Slice& lower, const Slice& upper) override 
    {
        auto itr = list.iterator();
        if(!lower.empty()) {
            itr.seek(Key(lower));
        } else {
            itr.seekToFirst();
        }
        return LookupRef(new SkipListIterator(SegmentRef(weakRef),itr,Key(lower),Key(upper),keyValueCompare(options)));
    }
    void close() override {}
    void removeSegment() override {
        close();
        fs::remove(path);
    }
    std::vector<std::string> files() override {
        return {fs::path(path).filename()};
    }
};
