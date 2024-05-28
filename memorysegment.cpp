#include "memorysegment.h"

void MemorySegment::maybeCreateLogFile() {
    if(log) return;
    if(path=="") return;
    log = new LogFile(path,id,options);
}

void MemorySegment::write(const WriteBatch& batch){
    maybeCreateLogFile();
    if(log) {
        log->startBatch(batch.size());
    }
    for(auto kv : batch.entries) {
        auto prev = list.put(kv);
        bytes += kv.key.length + kv.value.length - prev.key.length - prev.value.length;
        if(log) {
            log->write(kv.key,kv.value);
        }
    }
    if(log) {
        log->endBatch(batch.size());
    }
}