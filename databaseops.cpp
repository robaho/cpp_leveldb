#include "keyvalue.h"
#include "database.h"
#include "exceptions.h"
#include "memorysegment.h"
#include "multisegment.h"
#include "segment.h"

ByteBuffer Database::get(const Slice& key) {
    ByteBuffer buffer;
    return get(key,buffer);
}

ByteBuffer& Database::get(const Slice& key,ByteBuffer& value) {
    if(!_open) throw DatabaseClosed();
    if(key.length<=0 || key.length>1024) throw InvalidKeyLength();
    return getState()->multi->get(key,value);
}

void Database::maybeMerge() {
    if(options.disableAutoMerge) return;
    auto state = getState();
    if(state->segments.size()> 2*options.maxSegments) {
        merger.mergeSegments0(this,options.maxSegments);
    }
}

void Database::maybeSwapMemory() {
    auto state = getState();
    if(state->memory->size() > options.maxMemoryBytes) {
        auto segments = copyAndAppend(state->segments,state->memory);
        auto memory = MemorySegment::newMemorySegment(path,nextSegmentID(),options);
        auto multi = MultiSegment::newMultiSegment(copyAndAppend(segments,memory));
        setState(DatabaseState(segments,memory,multi));
    }
}

void Database::put(const Slice& key,const Slice& value) {
    DB_LOCK();

    if(!_open) throw DatabaseClosed();
    if(key.length<=0 || key.length>1024) throw InvalidKeyLength();

    maybeSwapMemory();

    getState()->memory->put(key,value);

    DB_UNLOCK();

    maybeMerge();
}

ByteBuffer Database::remove(const Slice& key) {
    DB_LOCK();
    if(!_open) throw DatabaseClosed();
    if(key.length <1 || key.length> 1024) throw InvalidKeyLength();

    auto value = get(key);
    if(value.empty()) return value;

    maybeSwapMemory();
    getState()->memory->remove(key);

    DB_UNLOCK();

    maybeMerge();

    return value;
}

void Database::write(const WriteBatch& batch) {
    {
        DB_LOCK();
        if(!_open) throw DatabaseClosed();
        maybeSwapMemory();
        getState()->memory.memory()->write(batch);
    }
    maybeMerge();
}

LookupRef Database::lookup(const Slice& lower,const Slice& upper) {
    auto snapshot = Database::snapshot();
    return snapshot->lookup(lower,upper);
}

SnapshotRef Database::snapshot() {
    DB_LOCK();
    if(!_open) throw DatabaseClosed();

    auto state = getState();
    auto segments = copyAndAppend(state->segments,state->memory);
    auto memory = MemorySegment::newMemorySegment(path,nextSegmentID(),options);
    auto multi = MultiSegment::newMultiSegment(copyAndAppend(segments,memory));

    setState(DatabaseState(segments,memory,multi));

    return SnapshotRef(new Snapshot(DatabaseRef(weakRef),MultiSegment::newMultiSegment(segments)));
}
