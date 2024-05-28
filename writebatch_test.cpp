#define BOOST_TEST_MODULE writebatch
#include <boost/test/included/unit_test.hpp>

#include "writebatch.h"

BOOST_AUTO_TEST_CASE( writebatch ) {
    WriteBatch batch;
    batch.put("mykey","myvalue");
    batch.put("mykey2","myvalue2");
    batch.put("mykey3","myvalue3");

    BOOST_TEST(batch.size()==3);
    batch.remove("mykey2");
    BOOST_TEST(batch.size()==2);
    batch.remove("mykey4");
    BOOST_TEST(batch.size()==2);
}

