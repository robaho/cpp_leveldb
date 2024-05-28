#define BOOST_TEST_MODULE lockfile
#include <boost/test/included/unit_test.hpp>

#include "lockfile.h"

BOOST_AUTO_TEST_CASE( lockfile ) {
    LockFile l("testfile");
    BOOST_TEST(l.tryLock());

    LockFile l2("testfile");
    BOOST_TEST(!l2.tryLock());

    l.unlock();

    BOOST_TEST(l2.tryLock());
}

BOOST_AUTO_TEST_CASE( lockfile_raii ) {
    {
        LockFile l("testfile");
        BOOST_TEST(l.tryLock());
    }

    LockFile l("testfile");
    BOOST_TEST(l.tryLock());
}

