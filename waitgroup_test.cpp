#define BOOST_TEST_MODULE waitgroup
#include <boost/test/included/unit_test.hpp>

#include "waitgroup.h"

BOOST_AUTO_TEST_CASE( waitgroup ) {
    WaitGroup wg;
    wg.add(1);
    BOOST_TEST(wg.count()==1);
    wg.done();
    BOOST_TEST(wg.count()==0);
}

BOOST_AUTO_TEST_CASE( waitgroup_done ) {
    WaitGroup wg;
    wg.add(1);
    {
        WaitGroupDone done(wg);
        BOOST_TEST(wg.count()==1);
    }
    BOOST_TEST(wg.count()==0);
}

BOOST_AUTO_TEST_CASE( waitgroup_use ) {
    WaitGroup wg;
    {
        UseWaitGroup use(wg);
        BOOST_TEST(wg.count()==1);
    }
    BOOST_TEST(wg.count()==0);
}