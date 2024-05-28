#ifndef _SEGMENT
#define _SEGMENT
#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstdio>
#include <atomic>
#include <memory>
#include <typeinfo>

#include "constants.h"
#include "bytebuffer.h"
#include "lookupiterator.h"

class Segment;

typedef std::shared_ptr<Segment> SegmentRef;


class Segment {
protected:
    bool shouldRemove = false;
public:
    Segment() : weakRef(){
    }
    virtual ~Segment() {
        // std::cout << "[segment destroyed] ";
    }
    virtual ID upperID() = 0;
    virtual ID lowerID() = 0;
    virtual ByteBuffer put(const Slice& key, const Slice& value) = 0;
    virtual ByteBuffer& get(const Slice& key,ByteBuffer &value) = 0;
    ByteBuffer get(const Slice& key) {
        ByteBuffer value;
        return get(key,value);
    }
    virtual ByteBuffer remove(const Slice& key) = 0;
    virtual void close() = 0;
    virtual LookupRef lookup(const Slice& lower, const Slice& upper) = 0;
    virtual void removeSegment() = 0;
    virtual void removeOnFinalize() { shouldRemove = true; }
    virtual std::vector<std::string> files() = 0;
    virtual uint64_t size() = 0;

    // used to obtain SegmentRef instances
    std::weak_ptr<Segment> weakRef;
};

typedef std::shared_ptr<Segment> SegmentRef;

static std::vector<SegmentRef> copyAndAppend(std::vector<SegmentRef> list, SegmentRef segment) {
    std::vector<SegmentRef> copy(list.size() + 1);
    copy = list;
    copy.push_back(segment);
    return copy;
}

#endif
