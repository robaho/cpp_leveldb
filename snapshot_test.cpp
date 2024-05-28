#define BOOST_TEST_MODULE snashot
#include <boost/test/included/unit_test.hpp>

#include <filesystem>

#include "database.h"

namespace fs = std::filesystem;

BOOST_AUTO_TEST_CASE( snapshot ) {
    Options options(true);
    options.disableAutoMerge=true;

    try { Database::remove("test/mydb"); } catch(DatabaseNotFound){}
    
    auto db = Database::open("test/mydb",options);
    db->put("mykey","myvalue");
    auto s = db->snapshot();
    db->put("mykey1","myvalue1");

    BOOST_TEST(s->get("mykey")=="myvalue");
    BOOST_TEST(s->get("mykey1").empty());
}
