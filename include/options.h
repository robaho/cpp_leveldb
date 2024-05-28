#ifndef _OPTIONS
#define _OPTIONS

#include <vector>
#include "bytebuffer.h"

class Options {
public:
    enum BatchReadMode {
        discardPartial,
        applyPartial,
        returnOpenError
    };
    // If true, then if the database does not exist on Open() it will be created.
    bool createIfNeeded = false;
    // The database segments are periodically merged to enforce MaxSegments.
    // If this is true, the merging only occurs during Close().
    bool disableAutoMerge = false;
    // Maximum number of segments per database which controls the number of open files.
    // If the number of segments exceeds 2x this value, producers are paused while the
    // segments are merged.
    int maxSegments = 0;
    // Maximum size of memory segment in bytes. Maximum memory usage per database is
    // roughly MaxSegments * MaxMemoryBytes but can be higher based on producer rate.
    int maxMemoryBytes = 0;
    // Disable flush to disk when writing to increase performance.
    bool disableWriteFlush = false;
    // Force sync to disk when writing. If true, then DisableWriteFlush is ignored.
    bool enableSyncWrite = false;
    // Determines handling of partial batches during Open()
    BatchReadMode batchReadMode = BatchReadMode::discardPartial;
    // Key comparison function or nil to use standard bytes.Compare
    int (*userKeyCompare)(const Slice& a,const Slice& b) = nullptr;

    Options() {
    }
    Options(bool createIfNeeded) {
        this->createIfNeeded = createIfNeeded;
    }
};
#endif

