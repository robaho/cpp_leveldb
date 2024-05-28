#pragma once

#include <algorithm>
#include <cstdint>
#include <string>

/**
 * @brief a Slice is a temporary view into an array of bytes. A slice must
 * not last beyond the ptr reference!
 */
struct Slice {
    const uint8_t *ptr;
    int length;
    Slice() : ptr(nullptr), length(0){};
    Slice(const uint8_t* ptr,int length) : ptr(ptr), length(length){};
    Slice(const char chars[]) : ptr((const uint8_t*)chars), length(strlen((const char *)ptr)){};
    Slice(const std::string s) : Slice(s.c_str()){};
    Slice(const Slice& slice) {
        ptr = slice.ptr;
        length = slice.length;
    }
    Slice(Slice&& slice) {
        ptr = slice.ptr;
        length = slice.length;
        slice.ptr = nullptr;
        slice.length = 0;
    }
    inline int compareTo(const Slice& other) const {
        if(ptr==other.ptr && (ptr==nullptr || length==other.length)) return 0;
        int min = std::min(length,other.length);
        int result = memcmp(ptr,other.ptr,min);
        if(result!=0) return result;
        return length - other.length;
    }
    operator uint8_t*() const { return (uint8_t *)ptr; }
    Slice& operator=(const Slice& other) {
        ptr=other.ptr;
        length = other.length;
        return *this;
    }
    inline bool operator==(Slice& other) const {
        return compareTo(other) == 0;
    }
    inline static bool less(const Slice& a, const Slice& b) {
        return a.compareTo(b) < 0;
    }
    Slice slice(int offset) const {
        return Slice(ptr+offset,length-offset);
    }
    Slice slice(int offset,int length) const {
        return Slice(ptr+offset,length);
    }
    bool empty() const { return ptr==nullptr || length==0; }
};
