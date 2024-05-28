#ifndef _WRITE_BATCH
#define _WRITE_BATCH

#include <list>
#include "keyvalue.h"

struct WriteBatch {
    std::list<const KeyValue> entries;
    void put(const ByteBuffer& key,const ByteBuffer& value) {
        entries.push_back(KeyValue(key,value));
    }
    void remove(const ByteBuffer& key) {
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

#endif