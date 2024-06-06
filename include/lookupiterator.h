#pragma once

#include <memory>
#include "keyvalue.h"

class LookupIterator {
    public:
        virtual Slice peekKey() = 0;
        KeyValue next() {
            KeyValue kv;
            return next(kv);
        }
        virtual ~LookupIterator(){};
        virtual KeyValue& next(KeyValue& kv) = 0;
};

struct EmptyIterator : public LookupIterator {
    Slice peekKey() { return Slice(); }
    KeyValue& next(KeyValue& kv) {
        kv = KeyValue::EMPTY();
        return kv;
    }
    ~EmptyIterator(){}
};

typedef std::unique_ptr<LookupIterator> LookupRef;
