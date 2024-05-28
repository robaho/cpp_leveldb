#include <string>
#include <format>
#include <filesystem>
#include <fstream>

#include "database.h"
#include "diskio.h"
#include "bytebuffer.h"

namespace fs = std::filesystem;

uint16_t readLEuint16(const unsigned char *buffer) {
    return buffer[0] + (buffer[1]<<8);
}
uint32_t readLEuint32(const unsigned char *buffer) {
    return buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
}
uint64_t readLEuint64(const unsigned char *buffer) {
    return readLEuint32(buffer) + (uint64_t(readLEuint32(buffer+4)) << 32);
}

uint16_t readLEuint16(std::istream& file) {
    return file.get() + (file.get()<<8);
}
uint32_t readLEuint32(std::istream& file) {
    return file.get() + (file.get() << 8) + (file.get() << 16) + (file.get() << 24);
}
uint64_t readLEuint64(std::istream& file) {
    return readLEuint32(file) + (uint64_t(readLEuint32(file)) << 32);
}

void writeSegmentToDisk(Database *db,SegmentRef seg) {
    auto itr = seg->lookup(ByteBuffer::EMPTY,ByteBuffer::EMPTY);
    if(itr->peekKey().empty()) {
        seg->removeSegment();
        return;
    }

    auto lowerId = seg->lowerID();
    auto upperId = seg->upperID();

    std::string keyFilename = db->path+"/keys."+std::to_string(lowerId)+"."+std::to_string(upperId);
    std::string dataFilename = db->path+"/data."+std::to_string(lowerId)+"."+std::to_string(upperId);
    writeAndLoadSegment(keyFilename,dataFilename,itr,false);
    seg->removeSegment();
}

SegmentRef writeAndLoadSegment(std::string keyFilename,std::string dataFilename,LookupRef itr,bool purgeDeleted) {
    if(fs::exists(keyFilename) || fs::exists(dataFilename)) {
        throw IllegalState("key/data file should not exist");
    }

    std::string keyFilenameTmp = keyFilename+".tmp";
    std::string dataFilenameTmp = dataFilename+".tmp";

    auto keyIndex = writeSegmentFiles(keyFilenameTmp,dataFilenameTmp,itr,purgeDeleted);

    fs::rename(keyFilenameTmp,keyFilename);
    fs::rename(dataFilenameTmp,dataFilename);

    return DiskSegment::newDiskSegment(keyFilename,dataFilename,keyIndex);
}

void writeLEuint16(ostream& fs,uint16_t value) {
    fs.put(value);
    fs.put(value>>8);
}
void writeLEuint32(ostream& fs,uint32_t value) {
    fs.put(value);
    fs.put(value>>8);
    fs.put(value>>16);
    fs.put(value>>24);
}
void writeLEuint64(ostream& fs,uint64_t value) {
    writeLEuint32(fs, value);
    writeLEuint32(fs, value>>32);
}

void writeBuffer(ostream& fs,const Slice& bb) {
    fs.write((const char*)(uint8_t*)bb,bb.length);
}
void writeBuffer(ostream& fs,const Slice& bb,int len) {
    fs.write((const char*)(uint8_t*)bb,len);
}

struct DiskKey {
    uint16_t keylen;
    Slice compressedKey;
};

int calculatePrefixLen(const Slice& prevKey,const Slice& key) {
    if(prevKey.empty()) return 0;
    int length = 0;
    for(;length < prevKey.length && length < key.length; length++) {
        if(prevKey[length] != key[length]) break;
    }
    if(length > maxPrefixLen || key.length-length > maxCompressedLen) {
        length = 0;
    }
    return length;
}

DiskKey encodeKey(const Slice& key,const Slice& prevKey) {
    int prefixLen = calculatePrefixLen(prevKey,key);
    if(prefixLen>0) {
        Slice suffix = key.slice(prefixLen);
        return DiskKey{.keylen=uint16_t(compressedBit| (prefixLen<<8) | suffix.length), .compressedKey = suffix};
    }
    return DiskKey{.keylen=(uint16_t)key.length, .compressedKey=key};
}

KeyIndex writeSegmentFiles(std::string keyFilename,std::string dataFilename,LookupRef itr,bool purgeDeleted) {
    KeyIndex keyIndex;

    std::fstream keyF;
    std::fstream dataF;

    keyF.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    dataF.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    keyF.open(keyFilename,fstream::out | fstream::binary);
    dataF.open(dataFilename,fstream::out | fstream::binary);

    int64_t dataOffset = 0;
    int keyBlockLen = 0;
    int keyCount = 0;
    int block = 0;

    ByteBuffer zeros(keyBlockSize);
    memset((uint8_t*)zeros,0,keyBlockSize);
    ByteBuffer prevKey;

    while(true) {
        auto kv = itr->next();
        if(kv.key.empty()) break;
        if(purgeDeleted && kv.value.empty()) continue;
        keyCount++;
        writeBuffer(dataF,kv.value);
        if(keyBlockLen+2+kv.key.length+8+4 >= keyBlockSize-2) {
            writeLEuint16(keyF,endOfBlock);
            keyBlockLen+=2;
            writeBuffer(keyF,zeros,keyBlockSize-keyBlockLen);
            keyBlockLen=0;
            prevKey = ByteBuffer::EMPTY;
        }
        if(keyBlockLen==0) {
            if((block%keyIndexInterval) == 0) {
                keyIndex.push_back(kv.key);
            }
            block++;
        }

        auto dk = encodeKey(kv.key,prevKey);
        prevKey = kv.key;
        writeLEuint16(keyF,dk.keylen);
        writeBuffer(keyF,dk.compressedKey);
        writeLEuint64(keyF,dataOffset);
        writeLEuint32(keyF,kv.value.length);

        dataOffset += kv.value.length;

        keyBlockLen += (2 + dk.compressedKey.length + 8 + 4);
    }

    if(keyBlockLen > 0 && keyBlockLen < keyBlockSize) {
        writeLEuint16(keyF,endOfBlock);
        keyBlockLen += 2;
        writeBuffer(keyF,zeros,keyBlockSize-keyBlockLen);
    }

    keyF.flush();
    dataF.flush();

    return keyIndex;
}