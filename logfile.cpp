#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <vector>

#include "skiplist.h"
#include "keyvalue.h"
#include "options.h"
#include "diskio.h"
#include "logfile.h"

LogFile::LogFile(const std::string &dbpath, uint64_t id, const Options &options) : 
    path(dbpath+"/log."+std::to_string(id)), options(options), file(&filebuf)
{
    if(dbpath=="") return;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    filebuf.open(path, O_CREAT | O_WRONLY | O_TRUNC | (options.enableSyncWrite ? O_SYNC : 0));

    if(!options.enableSyncWrite && options.disableWriteFlush) {
		disableFlush = true;
	}
}

void LogFile::startBatch(int len) {
    inBatch = true;
    writeLEuint32(file,-len);
}
void LogFile::endBatch(int len) {
    inBatch=false;
    writeLEuint32(file,-len);
    file.flush();
}
void LogFile::write(const ByteBuffer& key,const ByteBuffer& value) {
    writeLEuint32(file, key.length);
    file.write((char*)(uint8_t*)key,key.length);
    writeLEuint32(file, value.length);
    file.write((char*)(uint8_t*)value,value.length);
    if(!inBatch && !disableFlush) {
        file.flush();
    }
}
void LogFile::close() {
    file.flush();
    filebuf.close();
}

void LogFile::remove() {
    if(path!="") {
        std::filesystem::remove(path);
    }
}

void readLogFile(SkipList<const KeyValue> &list, std::string path, const Options &options)
{
    std::fstream file;

    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    file.open(path, std::fstream::in | std::fstream::binary);

    auto readBatch = [&](int32_t len)
    {
        std::vector<const KeyValue> entries;

        try
        {
            for (int i = 0; i < (len * -1); i++)
            {
                int32_t kLen = readLEuint32(file);
                ByteBuffer key(kLen);
                file.read((char *)(uint8_t *)key, kLen);
                int32_t vLen = readLEuint32(file);
                ByteBuffer value(vLen);
                file.read((char *)(uint8_t *)value, vLen);
                entries.push_back(KeyValue(key, value));
            }

            int32_t len0 = readLEuint32(file);
            if (len0 != len)
                throw DatabaseCorrupted();
        }
        catch (exception& ex)
        {
            if (options.batchReadMode != Options::BatchReadMode::applyPartial)
            {
                throw ex;
            }
        }
        for (auto &e : entries)
        {
            list.put(e);
        }
    };

    try
    {
        while (true)
        {
            file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

            int32_t len = readLEuint32(file);
            if (file.eof())
                return;

            file.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);

            if (len < 0)
            {
                readBatch(len);
            }
            else
            {
                uint32_t kLen = len;
                ByteBuffer key(kLen);
                file.read((char *)(uint8_t *)key, kLen);
                uint32_t vLen = readLEuint32(file);
                ByteBuffer value(vLen);
                file.read((char *)(uint8_t *)value, vLen);
                list.put(KeyValue(key, value));
            }
        }
    }
    catch (std::exception& ex)
    {
        if (options.batchReadMode == Options::BatchReadMode::returnOpenError)
        {
            throw ex;
        }
        return;
    }
}
