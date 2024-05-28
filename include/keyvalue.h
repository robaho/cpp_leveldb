#pragma once

#include <iostream>
#include <string>
#include <functional>

#include "bytebuffer.h"
#include "options.h"

struct KeyValue {
    ByteBuffer key;
    ByteBuffer value;
    static const KeyValue EMPTY;
    KeyValue(const KeyValue& kv) : key(kv.key) , value(kv.value){}
    KeyValue(const ByteBuffer& key,const ByteBuffer& value) : key(key), value(value){};
    KeyValue() : KeyValue(ByteBuffer::EMPTY,ByteBuffer::EMPTY) {}
    bool operator ==(const KeyValue& other) const {
        return key.compareTo(other.key)==0 && value.compareTo(other.value) == 0;
    }
    KeyValue& operator=(const KeyValue& other) {
        key=other.key;
        value=other.value;
        return *this;
    }
    operator std::string() const {
        return std::string(key) + "/" + std::string(value);
    }
};

typedef int (*CompareFn)(const KeyValue&,const KeyValue&);

#define Key(x) KeyValue(x,ByteBuffer::EMPTY)

static CompareFn keyValueCompare(const Options& options) {
	if (options.userKeyCompare == nullptr) {
		return [](const KeyValue&a,const KeyValue& b) -> int {
            return a.key.compareTo(b.key);
		};
	} else {
        static auto f = options.userKeyCompare;
        return [](const KeyValue& a,const KeyValue& b) -> int {
                return f(a.key,b.key);
        };
	}
}

extern std::ostream& operator<<(std::ostream& os, const KeyValue& kv);
