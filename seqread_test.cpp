#define BOOST_TEST_MODULE seqread
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <filesystem>
#include <chrono>

#include "database.h"

static const int nr = 1000000;
static const std::string dbname = "testdb/mydb";

static long millis(const std::chrono::system_clock::time_point& end,const std::chrono::system_clock::time_point& start) {
    return ((end-start).count())/1000;
}

static void _testRead() {
    auto db = Database::open(dbname,Options());
    auto start = std::chrono::system_clock::now();
    auto itr = db->lookup("","");
    int count = 0;
    KeyValue kv;
    while(!(itr->next(kv).key.empty())) count++;
    if(count!=nr) {
        std::cout << "count is " << count << "\n";
        throw IllegalState("incorrect count");
    }
    auto end = std::chrono::system_clock::now();
    auto ms = millis(end,start);
    std::cout << "read seq time " << ms << " ms, us per op " << (ms*1000)/(double)nr << "\n";
    db->close();
}

BOOST_AUTO_TEST_CASE( seqread ) {
    _testRead();
}
