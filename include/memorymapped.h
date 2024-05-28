#pragma once

#include <filesystem>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace fs = std::filesystem;
namespace bip = boost::interprocess;

/**
 * @brief memory mapped file using Boost
 */
class MemoryMappedFile {
private:
    fs::path path;
    const uint64_t _length;
    bip::file_mapping mapping;
    bip::mapped_region region;

public:
    MemoryMappedFile(std::string path) 
    :   path(path), 
        _length(fs::file_size(path)),
        mapping(path.c_str(), bip::read_only)
    {
        if(_length!=0) {
            region = bip::mapped_region(mapping, bip::read_only);
        }

    }
    std::string name() {
        return path.filename();
    }
    void close() {
    }
    void remove() {
        bip::file_mapping::remove(path.c_str());
    }
    uint64_t length() {
        return _length;
    }
    int readAt(unsigned char *buffer,uint64_t position,int length) {
        memcpy(buffer,(uint8_t*)(region.get_address())+position,length);
        return MIN(length,_length-position);
    }
};
