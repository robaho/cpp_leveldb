#include <vector>
#include <string>
#include <filesystem>
#include <boost/algorithm/string.hpp>
#include <memory>

#include <arpa/inet.h>

#include "disksegment.h"
#include "logsegment.h"
#include "diskio.h"

namespace fs = std::filesystem;

bool segmentCompare(const SegmentRef& a,const SegmentRef& b) {
    auto id1 = a->upperID();
    auto id2 = b->upperID();
    if(id1==id2) {
        return a->lowerID() > b->lowerID();
    }
    return id1 < id2;
}

std::vector<SegmentRef> DiskSegment::loadDiskSegments(std::string directory, Options options) {
    auto files = fs::directory_iterator(directory);
    std::vector<SegmentRef> segments;
    // first remove any tmp files 
    for(auto file : files) {
        if(file.path().extension()!=".tmp") continue;
        auto base = file.path().stem().string();
        std::string segs;
        if(base.starts_with("keys.")) {
            segs = base.substr(strlen("keys."));
        } else if(base.starts_with("data.")) {
            segs = base.substr(strlen("data."));
        } else {
            throw IllegalState(("unknown tmp file "+base).c_str());
        }
        auto removeFileIfExists = [](std::string filename) {
            fs::path path = filename;
            if(fs::exists(path)) fs::remove(path);
        };
        removeFileIfExists("keys."+segs);
        removeFileIfExists("data."+segs);
        removeFileIfExists("keys."+segs+".tmp");
        removeFileIfExists("data."+segs+".tmp");
    }
    files = fs::directory_iterator(directory);
    for(auto file : files) {
        if(file.path().string().starts_with("log.")) {
            auto ls = LogSegment::newLogSegment(file.path(),options);
            segments.push_back(ls);
        }
        auto filename = file.path().filename().string();
        if(!filename.starts_with("keys.")) continue;
        auto ids = getSegmentIDs(filename);
        auto keyFilename = directory+"/"+"keys."+std::to_string(ids.lower)+"."+std::to_string(ids.upper);
        auto dataFilename = directory+"/"+"data."+std::to_string(ids.lower)+"."+std::to_string(ids.upper);
        auto segment = DiskSegment::newDiskSegment(keyFilename,dataFilename,{});
        segments.push_back(segment);
    }
    std::sort(segments.begin(),segments.end(),segmentCompare);
    // remove any segments that are fully contained in another segment
next:
    for(auto i = segments.begin(); i<segments.end(); i++) {
        auto seg = *i;
        for(auto j = i + 1; j < segments.end(); j++) {
            auto seg0 = *j;
            if(seg->lowerID() >= seg0->lowerID() && seg->upperID() <= seg0->upperID()) {
                segments.erase(i);
                goto next;
            }
        }
    }
    return segments;
}

IDS getSegmentIDs(const std::string& filename) {
    std::vector<std::string> segs;
    boost::split(segs,filename,boost::is_any_of("."));
    return IDS{.lower = std::stoul(segs[1]), .upper= std::stoul(segs[2])};
}

/**
 * @brief search key file using binary search
 * 
 * @param key the key to look for
 * @param offset the offset of the value in the data file, or -1 if not found
 * @param length the length of the value in the data file
 */
void DiskSegment::binarySearch(const Slice& key,int64_t *offset,uint32_t *length) {
    int64_t lowBlock = 0;
    int64_t highBlock = keyBlocks - 1;

    auto itr = std::upper_bound(keyIndex.begin(),keyIndex.end(),key,Slice::less);
    if(itr!=keyIndex.begin()) itr--;

    auto index = itr-keyIndex.begin();

    lowBlock = index * keyIndexInterval;
    highBlock = lowBlock + keyIndexInterval;
    if(highBlock >= keyBlocks) {
        highBlock = keyBlocks-1;
    }

    ByteBuffer buffer(maxKeySize*2);
    int64_t block = binarySearch0(lowBlock,highBlock,key,buffer);
    scanBlock(block,key,offset,length);
}

/**
 * @brief recursive portion of binary search
 * 
 * @param lowBlock 
 * @param highBlock 
 * @param key 
 * @param buffer 
 * @return int64_t the block number to be scanned
 */
int64_t DiskSegment::binarySearch0(int64_t lowBlock,int64_t highBlock,const ByteBuffer& key,unsigned char *buffer) {
    if(highBlock-lowBlock<=1) {
        // the key is either in low block or high block, or does not exist, so check high block
        keyFile.readAt(buffer,highBlock*keyBlockSize,maxKeySize*2);
        uint16_t keylen = readLEuint16(buffer);

        Slice skey(buffer+2,keylen);

        if(Slice::less(key,skey)) {
            return lowBlock;
        } else {
            return highBlock;
        }
    }

    uint64_t block = (highBlock-lowBlock)/2 + lowBlock;
    keyFile.readAt(buffer,block*keyBlockSize,maxKeySize*2);
    uint16_t keylen = readLEuint16(buffer);
    Slice skey(buffer+2,keylen);

    if(Slice::less(key,skey)) {
        return binarySearch0(lowBlock,block,key,buffer);
    } else {
        return binarySearch0(block,highBlock,key,buffer);
    }
}

/**
 * @brief scan a key file block for the key
 * 
 * @param block the block number to scan
 * @param key the key to look for
 * @param offset the offset of the value in the data file, or -1 if the key was not found
 * @param length the length of the value in the data file
 */
void DiskSegment::scanBlock(int64_t block,const Slice& key,int64_t *offset,uint32_t *length) {
    uint8_t buffer[keyBlockSize];
    uint8_t tmpKey[maxKeySize];
    keyFile.readAt(buffer,block*keyBlockSize,keyBlockSize);
    int index = 0;

    Slice prevKey;

    while(true) {
        if(index+2>keyBlockSize) throw IllegalState("buffer overrun");

        uint16_t keylen = readLEuint16(buffer+index);
        int compressedLen = keylen;
        int prefixLen = 0;

        if(keylen==endOfBlock) {
            *offset=-1;
            return;
        }

        index+=2;

        if((keylen&compressedBit) != 0) {
            prefixLen = (keylen >> 8) & maxPrefixLen;
            compressedLen = keylen & maxCompressedLen;
        }

        int endOfKey = index + compressedLen;

        if(endOfKey>keyBlockSize) throw IllegalState("buffer overrun");

        if(prefixLen>0) {
            memcpy(tmpKey,prevKey,prefixLen);
        }
        memcpy(tmpKey+prefixLen,buffer+index,compressedLen);

        Slice candidate = Slice(tmpKey,prefixLen+compressedLen);
        
        if(candidate.compareTo(key)==0) {
            *offset =readLEuint64(buffer+endOfKey);
            *length =readLEuint32(buffer+endOfKey+8);
            return;
        }
        if(!Slice::less(candidate,key)) {
            *offset=-1;
            return;
        }
        prevKey = candidate;
        index = endOfKey + 12;
    }

}

ByteBuffer& DiskSegment::get(const Slice& key,ByteBuffer &value) {
    int64_t offset;
    uint32_t length;

    binarySearch(key,&offset,&length);

    if(offset<0 || length==0) {
        value = ByteBuffer::EMPTY;
        return value;
    }
    value.ensureCapacity(length);
    dataFile.readAt(value, offset, length);
    value.length = length;
    return value;
}

struct DecodedKeyLen {
    uint16_t prefixLen;
    uint16_t compressedLen;
};

DecodedKeyLen decodeKeyLen(uint16_t keylen) {
    uint16_t prefixLen;
    uint16_t compressedLen;
    if((keylen & compressedBit)!=0) {
        prefixLen = (keylen >> 8) & maxPrefixLen;
        compressedLen = keylen & maxCompressedLen;
        if(prefixLen > maxPrefixLen || compressedLen > maxCompressedLen) throw IllegalState("invalid prefix/compressed length");
    } else {
        if(keylen > maxKeySize) throw IllegalState("key > 1024");
        prefixLen = 0;
        compressedLen = keylen;
    }
    if(compressedLen==0) throw IllegalState("compressed lkey length = 0");
    return DecodedKeyLen{.prefixLen = prefixLen, .compressedLen = compressedLen};
}

void decodeKey(ByteBuffer &buffer,const Slice& key, const Slice& prevKey,uint16_t prefixLen) {
    buffer = prevKey.slice(0,prefixLen);
    buffer.append(key);
}

void DiskSegment::Iterator::nextKeyValue() {
    if(finished) {
        currentKey = ByteBuffer::EMPTY;
        return;
    }

    DiskSegment *dsp = (DiskSegment*)ds.get();

    prevKey = currentKey;
    while(true) {
        uint16_t keylen = readLEuint16(buffer+bufferOffset);
        bufferOffset += 2;
        if(keylen == endOfBlock) {
            block++;
            if(block == dsp->keyBlocks) {
                finished = true;
                currentKey = ByteBuffer::EMPTY;
                valid = true;
                return;
            }
            int n = dsp->keyFile.readAt(buffer,block*keyBlockSize,keyBlockSize);
            if(n!=keyBlockSize) throw IllegalState("did not read key block size");
            bufferOffset = 0;
            prevKey = ByteBuffer::EMPTY;
            continue;
        }
        DecodedKeyLen dk = decodeKeyLen(keylen);
        Slice keySlice = buffer.slice(bufferOffset,dk.compressedLen);
        bufferOffset += dk.compressedLen;
        decodeKey(currentKey,keySlice,prevKey,dk.prefixLen);
        uint64_t dataoffset = readLEuint64(buffer+bufferOffset);
        bufferOffset+=8;
        uint32_t datalen = readLEuint32(buffer+bufferOffset);
        bufferOffset+=4;
        prevKey = currentKey;
        if(!lower.empty()) {
            if(ByteBuffer::less(currentKey,lower))
                continue;
            if(currentKey.compareTo(lower)==0) goto found;
        }
        if(!upper.empty()) {
            if(currentKey.compareTo(upper)==0) goto found;
            if(!ByteBuffer::less(currentKey,upper)) {
                finished=true;
                valid=true;
                currentKey = ByteBuffer::EMPTY;
                return;
            }
        }
    found:
        if(datalen==0) {
            currentValue = ByteBuffer::EMPTY;
        } else {
            currentValue.ensureCapacity(datalen);
            dsp->dataFile.readAt(currentValue,dataoffset,datalen);
            currentValue.length = datalen;
        }
        valid=true;
        return;
    }
}

void DiskSegment::loadKeyIndex(std::vector<const ByteBuffer>& index,MemoryMappedFile& kf,int64_t keyBlocks) {
    ByteBuffer buffer(keyBlockSize);
    if(kf.length()==0) return;
    int64_t block;
    for(block = 0; block < keyBlocks; block+= keyIndexInterval) {
        kf.readAt(buffer,block*keyBlockSize,keyBlockSize);
        uint16_t keylen = readLEuint16(buffer);
        if(keylen==endOfBlock) {
            break;
        }
        ByteBuffer key(keylen);
        memcpy(key,buffer+2,keylen);
        index.push_back(key);
    }
}
