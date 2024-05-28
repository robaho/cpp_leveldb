#pragma once

#include <vector>

class Arena {
private:
    static const int constexpr BLOCK_SIZE = 4*1024;
    std::vector<uint8_t *> blocks;
    int used=0;
    uint8_t *block = nullptr;
    inline int remaining() { return BLOCK_SIZE-used; }
public:
    Arena(){}
    ~Arena() {
        for(auto b : blocks) {
            delete[] b;
        }
    }
    void* allocate(int len) {
        const int align = (sizeof(void*) > 8) ? sizeof(void*) : 8;

        if(len > BLOCK_SIZE/4) {
            auto p = new uint8_t[BLOCK_SIZE];
            blocks.push_back(p);
            return p;
        }
        if(block==nullptr || len > remaining()) {
            block = new uint8_t[BLOCK_SIZE];
            used=0;
            blocks.push_back(block);
        }
        auto result = block+used;
        int slop = len%align;
        used+=(len+(slop == 0 ? 0 : align - slop));
        return result;
    }    
};
