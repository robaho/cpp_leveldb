
#include "skiplist.h"
#include "keyvalue.h"

int compareKey(const KeyValue& a,const KeyValue& b) {
    return a.key.compareTo(b.key);
}

void dotest() {
    SkipList<KeyValue> list(compareKey);
    list.put(KeyValue("mykey","myvalue"));
}

int main(int argc,const char **argv) {
    dotest();
}