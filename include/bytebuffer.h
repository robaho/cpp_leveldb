#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "exceptions.h"
#include "slice.h"

/**
 * @brief a variable length buffer similar to a vector but with support for Slice views
 */
class ByteBuffer {
    private:
        uint8_t *ptr;
        int capacity_;
        ByteBuffer(const uint8_t* chars,int length) : ByteBuffer(length) {
            memcpy((void *)ptr,(const void*)chars,length);
        }
    public:
        ~ByteBuffer() { free(ptr); }
        ByteBuffer() : ByteBuffer(64) { length = 0;}
        ByteBuffer(int len) : ptr(len==0 ? nullptr : len<0 ? throw IllegalState("length must be >=0") : new uint8_t[len]), capacity_(len), length(len)  {};
        ByteBuffer(const ByteBuffer& bb) : ByteBuffer(bb.ptr,bb.length){}
        ByteBuffer(const Slice& slice) : ByteBuffer(slice,slice.length){};
        ByteBuffer(ByteBuffer&& bb) : ptr(bb.ptr), capacity_(bb.capacity_), length(bb.length) { bb.ptr=nullptr; }
        ByteBuffer(const char* chars) : ByteBuffer(strlen(chars)) {
            memcpy((void *)ptr,(const void*)chars,length);
        }
        ByteBuffer(const std::string s) : ByteBuffer(s.c_str()) {}
        ByteBuffer& operator=(const ByteBuffer& other) {
            if(capacity_<other.length) {
                ptr = (uint8_t*)realloc(ptr, other.length);
                capacity_ = other.length;
            }
            memcpy(ptr,other.ptr,other.length);
            length = other.length;
            return *this;
        }
        ByteBuffer& operator=(const Slice& other) {
            if(capacity_<other.length) {
                ptr = (uint8_t*)realloc(ptr, other.length);
                capacity_ = other.length;
            }
            memcpy(ptr,other.ptr,other.length);
            length = other.length;
            return *this;
        }
        Slice slice() const {
            return Slice(ptr,length);
        }
        Slice slice(int offset) const {
            return Slice(ptr+offset,length-offset);
        }
        Slice slice(int offset,int length) const {
            if(offset+length > this->length) throw IllegalState("slice outside of bounds");
            return Slice(ptr+offset,length);
        }
        int length;
        bool empty() const { return length==0; }
        static const ByteBuffer EMPTY;
        int compareTo(const ByteBuffer& other) const {
            return slice().compareTo(other.slice());
        }
        operator uint8_t*() const { return (uint8_t *)ptr; }
        operator std::string() const {
            return length==0 ? "" : std::string((const char*)ptr,length);
        }
        operator Slice() const {
            return Slice(ptr,length);
        }
        void ensureCapacity(int capacity) {
            if(capacity_<capacity) {
                ptr = (uint8_t*)realloc(ptr,capacity);
                capacity_ = capacity;
            }
        }
        void append(const Slice& slice) {
            ensureCapacity(length+slice.length);
            memcpy(ptr+length,slice,slice.length);
            length+=slice.length;
        }
        bool operator ==(const ByteBuffer& other) const {
            return compareTo(other) == 0;
        }
        static bool less(const ByteBuffer& a, const ByteBuffer& b) {
            return a.compareTo(b) < 0;
        }
};
