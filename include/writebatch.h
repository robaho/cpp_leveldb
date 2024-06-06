#pragma once

#include <list>
#include "keyvalue.h"

struct WriteBatch {
    std::list<const KeyValue> entries;
    void put(const Slice& key,const Slice& value) {
        entries.push_back(KeyValue(key,value));
    }
    void remove(const Slice& key) {
        for(auto itr = entries.begin(); itr != entries.end(); itr++) {
            if(itr->key.compareTo(key)==0) {
                entries.erase(itr);
                break;
            }
        }
    }
    int size() const {
        return entries.size();
    }
};
