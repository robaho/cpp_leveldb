#include "database.h"
#include "bytebuffer.h"
#include "exceptions.h"

ByteBuffer Snapshot::get(const Slice& key) {
    ByteBuffer buffer;
    return get(key,buffer);
}

ByteBuffer& Snapshot::get(const Slice& key,ByteBuffer& value) {
    if(multi.get()==nullptr) throw SnapshotClosed();
    return multi->get(key,value);
}

LookupRef Snapshot::lookup(const Slice& lower,const Slice& upper) {
    if(multi.get()==nullptr) throw SnapshotClosed();
    return multi->lookup(lower,upper);
}