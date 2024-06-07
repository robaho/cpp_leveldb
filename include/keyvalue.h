#pragma once

#include <iostream>
#include <string>
#include <functional>

#include "bytebuffer.h"
#include "options.h"

struct KeyValue {
    ByteBuffer key;
    ByteBuffer value;
    static const KeyValue& EMPTY() {
        static const KeyValue kv(ByteBuffer::EMPTY(),ByteBuffer::EMPTY());
        return kv;
    };
    KeyValue(const KeyValue& kv) : key(kv.key) , value(kv.value){}
    KeyValue(const KeyValue& kv,Arena* arena) : key(kv.key,arena) , value(kv.value,arena){}
    KeyValue(const Slice& key,const Slice& value) : key(key), value(value){
        // std::cout << "allocated KeyValue\n";
    };
    KeyValue() : KeyValue(ByteBuffer::EMPTY(),ByteBuffer::EMPTY()) {
        // std::cout << "allocated empty KeyValue\n";
    }
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

#define Key(x) KeyValue(x,ByteBuffer::EMPTY())

inline CompareFn keyValueCompare(const Options& options) {
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

inline std::ostream& operator<<(std::ostream& os, const KeyValue& kv)
{
    return os << std::string(kv.key) << '/' << std::string(kv.value);
}
