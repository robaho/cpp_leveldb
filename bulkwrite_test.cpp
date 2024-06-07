#define BOOST_TEST_MODULE dbbench
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <filesystem>
#include <chrono>
#include <random>

#include "database.h"

#include "./test_helpers.h"

static const int nr = 1000000;
static const int vSize = 100;
static const int kSize = 16;
static const int batchSize = 1000;
static const std::string dbname = "testdb/mydb";

static uint8_t value[vSize];

static void _testWrite(bool sync,bool remove) {
    if(remove) {
        Database::remove(dbname);
    }

    ByteBuffer val(vSize);
    memcpy(val,value,vSize);

    int n = nr;
    if(sync) n = n / 1000;
    Options options(true);
    options.enableSyncWrite=sync;
    auto db = Database::open(dbname,options);
    auto start = std::chrono::system_clock::now();
    auto format = "%0"+std::to_string(kSize)+"d";

    for(int i=0;i<n;i++) {
        char tmp[kSize+1];
        snprintf(tmp,kSize+1,format.c_str(),i);
        db->put(tmp,val);
    }
    auto end = std::chrono::system_clock::now();
    std::string mode = std::string(sync ? "sync" : "no-sync") + (remove ? "" : " overwrite");
    auto ms = millis(end,start);
    auto usecPerOp = (ms*1000)/(double)n;
    std::cout << "write " << mode << " time " << ms << " records " << n << " usec per ops " << usecPerOp  << "\n"; 
    start = std::chrono::system_clock::now();
    db->closeWithMerge(0);
    end = std::chrono::system_clock::now();
    std::cout << "close time " << millis(end,start) << "\n";
    std::cout << "database size " << dbsize(dbname) << "\n";
}

BOOST_AUTO_TEST_CASE( bulkwrite ) {
    for(int i=0;i<vSize;i++) {
        value[i] = (uint8_t)rand();
    }
    _testWrite(false,true);
    _testWrite(false,false);
}
