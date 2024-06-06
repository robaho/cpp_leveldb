#pragma once

#include <string>

#include "constants.h"
#include "segment.h"
#include "bytebuffer.h"
#include "logfile.h"
#include "options.h"
#include "skiplist.h"
#include "keyvalue.h"
#include "lookupiterator.h"
#include "skiplistitr.h"
#include "writebatch.h"

class MemorySegment;

class MemorySegmentRef : public SegmentRef {
public:
    MemorySegmentRef(MemorySegment* ms) : SegmentRef((Segment*)ms){};
    MemorySegment* memory() const { return (MemorySegment*)SegmentRef::get();}
};

class MemorySegment final : public Segment
{
private:
    SkipList<const KeyValue> list;
    LogFile *log = nullptr;
    ID id;
    uint64_t bytes = 0;
    const std::string path;
    Options options;

    void maybeCreateLogFile();

    MemorySegment(const std::string& path, const uint64_t id,const Options& options) : list(keyValueCompare(options)), id(id), path(path), options(options) {}
public:
    ~MemorySegment() {
        if(shouldRemove) removeSegment();
    }
    static MemorySegmentRef newMemorySegment(const std::string& path,uint64_t id,const Options& options) 
    {
        auto ref = MemorySegmentRef(new MemorySegment(path, id, options));
        ref->weakRef = ref;
        return ref;
    }
    static MemorySegmentRef newMemoryOnlySegment()
    {
        return newMemorySegment("",0,Options());
    }
    ID upperID() override { return id; }
    ID lowerID() override { return id; }
    uint64_t size() override { return bytes; }
    ByteBuffer put(const Slice& key, const Slice& value) override
    {
        maybeCreateLogFile();
        auto prev = list.put(KeyValue(key,value));
        bytes += key.length + value.length - (prev.key.empty() ? 0 : (prev.key.length + prev.value.length));
        if(log) {
            log->write(key,value);
        }
        return prev.value;
    }
    ByteBuffer& get(const Slice& key,ByteBuffer& val) override
    {
        val = list.get(Key(key)).value;
        return val;
    }
    ByteBuffer remove(const Slice& key) override
    {
        return put(key, ByteBuffer::EMPTY());
    }
    void write(const WriteBatch& batch);

    void close() override {}
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
    void removeSegment() override
    {
        if(log!=nullptr) {
            log->remove();
            delete(log);
            log=nullptr;
        }
    }
    std::vector<std::string> files() override { return {}; }
};
