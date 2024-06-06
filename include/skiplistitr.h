#pragma once

#include "keyvalue.h"
#include "skiplist.h"
#include "segment.h"

class SkipListIterator final : public LookupIterator {
    const SegmentRef seg;
    SkipList<const KeyValue>::Iterator itr;
    const KeyValue lower;
    const KeyValue upper;
    const CompareFn cmp;
    KeyValue peek;
public:
    SkipListIterator(SegmentRef seg,SkipList<const KeyValue>::Iterator itr,const KeyValue& lower,const KeyValue& upper,CompareFn cmp) :
        seg(seg), itr(itr), lower(lower), upper(upper), cmp(cmp){}

    KeyValue& next(KeyValue& kv) override {
        if(!itr.valid()) {
            kv = KeyValue::EMPTY();
            return kv;
        }
        kv = peek.key.empty() ? itr.key() : peek;
        peek = KeyValue::EMPTY();
        if(!upper.key.empty() && cmp(kv,upper)>0) {
            kv = KeyValue::EMPTY();
            return kv;
        }
        itr.next();
        return kv;
    }
    Slice peekKey() override {
        if(!itr.valid()) return Slice();
        if(peek.key.empty()) peek = itr.key();
        if(!upper.key.empty() && cmp(peek,upper)>0) {
            return Slice();
        }
        return peek.key;
    }
};
