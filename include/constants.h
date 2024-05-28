#include <stddef.h>

#ifndef _CONSTANTS
#define _CONSTANTS
typedef unsigned long long ID;

const int keyBlockSize = 4096;
const int maxKeySize = 1000;
const uint16_t endOfBlock = 0x8000;
const uint16_t compressedBit = 0x8000;
const uint16_t maxPrefixLen = 0xFF ^ 0x80;
const uint16_t maxCompressedLen = 0xFF;
const int keyIndexInterval = 16;

#endif