#pragma once

#include <vector>
#include <memory.h>

#include "exceptions.h"
#include "segment.h"
#include "bytebuffer.h"
#include "memorymapped.h"

struct IDS {
    uint64_t lower;
    uint64_t upper;
};

ID getSegmentID(const std::string& filename);
IDS getSegmentIDs(const std::string& filename);

typedef std::vector<const ByteBuffer> KeyIndex;

#include "diskio.h"

class DiskSegment final : public Segment {
private:
    MemoryMappedFile keyFile;
    MemoryMappedFile dataFile;
    const int64_t keyBlocks;
    ID _lowerID;
    ID _upperID;
    KeyIndex keyIndex;

    DiskSegment(const std::string& keyFilename,const std::string& dataFilename, const KeyIndex& keyIndex)
        :   keyFile(keyFilename), dataFile(dataFilename), 
            keyBlocks((keyFile.length()-1)/keyBlockSize + 1),
            keyIndex(keyIndex)
    {
            auto ids = getSegmentIDs(keyFilename);
            _lowerID = ids.lower;
            _upperID = ids.upper;
            if(keyIndex.size()==0) {
                loadKeyIndex(this->keyIndex,keyFile,keyBlocks);
            }
    }
    static void loadKeyIndex(KeyIndex &index,MemoryMappedFile &keyFile,int64_t keyBlocks);

    void binarySearch(const Slice& key,int64_t *offset,uint32_t *length);
    int64_t binarySearch0(int64_t lowBlock,int64_t highBlock,const ByteBuffer& key,unsigned char *buffer);
    void scanBlock(int64_t block,const Slice& key, int64_t *offset,uint32_t *len);

    class Iterator final : public LookupIterator {
    private:
        bool valid=false;
        bool finished=false;
        ByteBuffer currentKey;
        ByteBuffer currentValue;
        const SegmentRef ds;
        uint64_t block;
        const ByteBuffer lower;
        const ByteBuffer upper;
        ByteBuffer buffer;
        ByteBuffer tmpKey,prevKey;
        int bufferOffset = 0;

        void nextKeyValue();

    public:
        Iterator(SegmentRef ds,const Slice& lower,const Slice& upper,const ByteBuffer& buffer, uint64_t block) :
            currentKey(64), ds(ds), block(block), lower(lower), upper(upper), buffer(buffer), tmpKey(64), prevKey(64) {
        }
        KeyValue& next(KeyValue& kv) override {
            if(valid) {
                valid=false;
                kv.key = currentKey;
                kv.value = currentValue;
                return kv;
            }
            nextKeyValue();
            valid=false;
            kv.key = currentKey;
            kv.value = currentValue;
            return kv;
        }
        Slice peekKey() override {
            if(valid) return currentKey;
            nextKeyValue();
            return currentKey;
        }
    };
public:
    static SegmentRef newDiskSegment(const std::string& keyFilename,const std::string& dataFilename,const KeyIndex& keyIndex) {
        auto ref = SegmentRef(new DiskSegment(keyFilename,dataFilename,keyIndex));
        ref->weakRef = ref;
        return ref;
    }
    ID lowerID() override { return _lowerID; }
    ID upperID() override { return _upperID; }
    uint64_t size() override { return keyFile.length() + dataFile.length(); }
    ByteBuffer put(const Slice& key,const Slice &value) override {
        throw IllegalState("disk segments are immutable, put is not allowed");
    }
    ByteBuffer remove(const Slice& key) override {
        throw IllegalState("disk segments are immutable, remove is not allowed");
    }
    void close() override {
        keyFile.close();
        dataFile.close();
    }
    void removeSegment() override {
        close();
        keyFile.remove();
        dataFile.remove();
    }
    std::vector<std::string> files() override {
        return { keyFile.name(), dataFile.name() };
    }
    LookupRef lookup(const Slice& lower, const Slice& upper) override {
        if(keyFile.length()==0) return LookupRef(new EmptyIterator());
        ByteBuffer buffer(keyBlockSize);
        int64_t block = 0;
        if(!lower.empty()) {
            auto itr = std::upper_bound(keyIndex.begin(),keyIndex.end(),lower,ByteBuffer::less);
            if(itr!=keyIndex.begin()) itr--;
            int index = itr - keyIndex.begin();
            block = index * keyIndexInterval;
        }
        int n = keyFile.readAt(buffer,block*keyBlockSize,keyBlockSize);
        if(n != keyBlockSize) {
            throw IllegalState("did not read block size");
        }
        return LookupRef(new Iterator(SegmentRef(weakRef),lower,upper,buffer,block));
    }
    ByteBuffer& get(const Slice& key,ByteBuffer &value) override;
    static std::vector<SegmentRef> loadDiskSegments(std::string directory,Options options);
};
