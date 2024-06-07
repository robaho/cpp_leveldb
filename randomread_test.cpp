#define BOOST_TEST_MODULE randomread
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <filesystem>
#include <chrono>
#include <random>

#include "database.h"

#include "./test_helpers.h"

static const int nr = 1000000;
static const int kSize = 16;
static const std::string dbname = "testdb/mydb";

static void _testReadRandom(DatabaseRef &db) {
    std::vector<int> keys(nr);
    for(int i=0;i<nr;i++) {
        keys.push_back(i);
    }

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(keys.begin(),keys.end(),g);

    auto start = std::chrono::system_clock::now();
    ByteBuffer value;
    auto format = ("%0"+std::to_string(kSize)+"d");
    for(int index : keys) {
        char tmp[kSize+1];
        snprintf(tmp,kSize+1,format.c_str(),index);
        auto val = db->get(tmp,value);
    }
    auto end = std::chrono::system_clock::now();
    auto ms = millis(end,start);
    std::cout << "read random time " << (ms*1000)/(double)(nr) << " us per get\n";
}

static void _testRead() {
    auto db = Database::open(dbname,Options());
    _testReadRandom(db);
    db->close();
}

BOOST_AUTO_TEST_CASE( randomread ) {
    _testRead();
}
