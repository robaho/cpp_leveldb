#define BOOST_TEST_MODULE disksegment
#include <boost/test/included/unit_test.hpp>

#include <filesystem>

#include "keyvalue.h"
#include "disksegment.h"
#include "memorysegment.h"

namespace fs = std::filesystem;

BOOST_AUTO_TEST_CASE( disksegment ) {
    fs::remove_all("test");
    fs::create_directory("test");
    auto ms = MemorySegment::newMemoryOnlySegment();
    ms->put("mykey","myvalue");
    ms->put("mykey2","myvalue2");
    ms->put("mykey3","myvalue3");
    auto itr = ms->lookup("","");
    auto ds = writeAndLoadSegment("test/keys.0.0","test/data.0.0",itr,false);
    auto itr2 = ds->lookup("","");
    int count=0;
    while(true) {
        auto kv = itr2->next();
        if(kv.key.empty()) break;
        count++;
    }
    BOOST_TEST(count==3);
    auto value = ds->get("mykey");
    BOOST_TEST(value.compareTo("myvalue")==0);
    value = ds->get("mykey2");
    BOOST_TEST(value.compareTo("myvalue2")==0);
    value = ds->get("mykey3");
    BOOST_TEST(value.compareTo("myvalue3")==0);
    value = ds->get("mykey4");
    BOOST_TEST(value.empty());
}

BOOST_AUTO_TEST_CASE( disksegment_empty ) {
    fs::remove_all("test");
    fs::create_directory("test");
    auto ms = MemorySegment::newMemoryOnlySegment();
    ms->put("mykey","myvalue");
    ms->remove("mykey");
    auto itr = ms->lookup("","");
    auto ds = writeAndLoadSegment("test/keys.0.0","test/data.0.0",itr,false);
    itr = ds->lookup("","");
    int count=0;
    while(true) {
        auto kv = itr->next();
        if(kv.key.empty()) break;
        count++;
    }
    // the segment must return the empty array for the key, so that removes are accurate in the case of multi segment multi
    BOOST_TEST(count==1);
    fs::remove_all("test");
}


BOOST_AUTO_TEST_CASE( disksegment_large ) {
    fs::remove_all("test");
    fs::create_directory("test");
    auto ms = MemorySegment::newMemoryOnlySegment();
    for(int i=0;i<1000000;i++) {
        std::string k = "mykey"+std::to_string(i);
        std::string v = "myvalue"+std::to_string(i);
        ms->put(k,v);
    }
    auto itr = ms->lookup("","");
    auto ds = writeAndLoadSegment("test/keys.0.0","test/data.0.0",itr,false);
    itr = ds->lookup("","");
    int count = 0;
    while(true) {
        auto kv = itr->next();
        if(kv.key.empty()) break;
        count++;
    }
    BOOST_TEST(count==1000000);
    auto value = ds->get("mykey1");
    BOOST_TEST(value=="myvalue1");
    value = ds->get("mykey1000000");
    BOOST_TEST(value.empty());
    itr = ds->lookup("mykey1","mykey1");
    auto kv = itr->next();
    BOOST_TEST(kv.key=="mykey1");
    BOOST_TEST(kv.value=="myvalue1");
}

