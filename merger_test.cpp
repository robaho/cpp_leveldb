#define BOOST_TEST_MODULE merger
#include <boost/test/included/unit_test.hpp>

#include <filesystem>
#include <vector>

#include "database.h"

namespace fs = std::filesystem;

BOOST_AUTO_TEST_CASE( merger ) {
    fs::remove_all("test");
    fs::create_directory("test");
    auto m1 = MemorySegment::newMemoryOnlySegment();
    for(int i=0;i<100000;i++) {
        m1->put("mykey"+std::to_string(i),"myvalue"+std::to_string(i));
    }
    auto m2 = MemorySegment::newMemoryOnlySegment();
    for(int i=100000;i<200000;i++) {
        m1->put("mykey"+std::to_string(i),"myvalue"+std::to_string(i));
    }

    Merger merger;
    Deleter deleter;

    auto segments = std::vector<SegmentRef>{m1,m2};

    auto merged = merger.mergeSegments1(deleter,"test",segments,false);
    auto itr = merged->lookup("","");
    int count=0;
    while(!itr->next().key.empty()) {
        count++;
    }
    BOOST_TEST( count==200000);
}
