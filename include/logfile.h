#pragma once

#include <string>
#include <filesystem>
#include <fstream>

#include "keyvalue.h"
#include "options.h"
#include "skiplist.h"
#include "fdfilebuf.h"

class LogFile {
    private:
        const std::string path;
        const uint64_t id;
        const Options options;
        FdFileBuf filebuf;
        std::ostream file;
        bool disableFlush=false;
        bool inBatch=false;
    public:
        void remove();
        void write(const ByteBuffer& key,const ByteBuffer& value);
        void startBatch(int len);
        void endBatch(int len);
        void close();
        LogFile(const std::string& dbpath,uint64_t id,const Options& options);
};

void readLogFile(SkipList<const KeyValue>& list,std::string path,const Options& options);
