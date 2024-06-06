#define BOOST_TEST_MODULE memorysegment
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <random>
#include <algorithm>

#include "skiplist.h"
#include "keyvalue.h"
#include "memorysegment.h"

BOOST_AUTO_TEST_CASE( memorysegment_put ) {
    auto ms = MemorySegment::newMemoryOnlySegment();
    ms->put("mykey","myvalue");
    BOOST_TEST( ms->get("mykey")=="myvalue");
    BOOST_TEST( ms->get("mykeyxxx").empty());
}
BOOST_AUTO_TEST_CASE( memorysegment_remove ) {
    auto ms = MemorySegment::newMemoryOnlySegment();
    ms->put("mykey","myvalue");
    auto r = ms->remove("mykey");
    BOOST_TEST( r=="myvalue");
    BOOST_TEST( ms->get("mykey").empty());
}
BOOST_AUTO_TEST_CASE( memorysegment_userkeyorder ) {
    auto fn = [](const Slice& a,const Slice& b) -> int {
        return -1 * a.compareTo(b);
    };
    Options options;
    options.userKeyCompare = fn;
    auto ms = MemorySegment::newMemorySegment("",0,options);
    ms->put("mykey1","myvalue1");
    ms->put("mykey2","myvalue2");

    auto itr = ms->lookup(ByteBuffer::EMPTY(),ByteBuffer::EMPTY());
    auto kv = itr->next();
    BOOST_TEST( kv.key=="mykey2");
    BOOST_TEST( kv.value=="myvalue2");
}
