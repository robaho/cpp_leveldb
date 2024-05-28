#ifndef _MERGER
#define _MERGER

#include "segment.h"

class Database;
class Deleter;

class Merger {
    std::mutex mtx;
    std::condition_variable cv;
    std::mutex merger;

public:
    void autoMerger(Database* db);
    void wakeup();
    void mergeSegments0(Database *db,int maxSegments);
    SegmentRef mergeSegments1(Deleter &deleter, const std::string& dbpath, std::vector<SegmentRef>& segments,bool purgeDeleted);
};

#endif

