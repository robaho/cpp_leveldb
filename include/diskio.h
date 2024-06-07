#pragma once

#include <memory>
#include <vector>
#include <stdint.h>

#include "database.h"
#include "disksegment.h"

uint16_t readLEuint16(const unsigned char *buffer);
uint32_t readLEuint32(const unsigned char *buffer);
uint64_t readLEuint64(const unsigned char *buffer);
uint16_t readLEuint16(std::istream& file);
uint32_t readLEuint32(std::istream& file);
uint64_t readLEuint64(std::istream& file);
void writeLEuint16(ostream& fs,uint16_t value);
void writeLEuint32(ostream& fs,uint32_t value);
void writeLEuint64(ostream& fs,uint64_t value);

void writeSegmentToDisk(Database *db,Segment *seg);
SegmentRef writeAndLoadSegment(std::string keyFilename,std::string dataFilename,LookupIterator *itr,bool purgeDeleted);
KeyIndex writeSegmentFiles(std::string keyFilename,std::string dataFilename,LookupIterator *itr,bool purgeDeleted);
