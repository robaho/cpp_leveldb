#define BOOST_TEST_MODULE disksegment
#include <boost/test/included/unit_test.hpp>

#include <string>
#include <filesystem>
#include <chrono>
#include <random>

#include "database.h"

static const int nr = 1000000;
static const int vSize = 100;
static const int kSize = 16;
static const int batchSize = 1000;
static const std::string dbname = "testdb/mydb";

static uint8_t value[vSize];

static long millis(const std::chrono::system_clock::time_point& end,const std::chrono::system_clock::time_point& start) {
    return ((end-start).count())/1000;
}

static std::string dbsize(const std::string& dbpath) {
    long size = 0;
    for(auto f : fs::directory_iterator(dbpath)) {
        size += fs::file_size(f);
    }
    char tmp[128];
    snprintf(tmp,128,"%.1ldM", size/(1024*1024));
    return tmp;
}

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
}

static void _testBatch() {

    ByteBuffer val(vSize);
    memcpy(val,value,vSize);

    Database::remove(dbname);
    Options options(true);
    options.maxSegments=64;
    options.disableAutoMerge=true;
    auto db = Database::open(dbname,options);
    auto start = std::chrono::system_clock::now();
    auto format = ("%0"+std::to_string(kSize)+"d");
    for(int i=0;i<nr;) {
        WriteBatch wb;
        for(int j=0;j<batchSize;j++) {
            char tmp[kSize+1];
            snprintf(tmp,kSize+1,format.c_str(),i+j);
            wb.put(tmp,val);
        }
        db->write(wb);
        i+=batchSize;
    }
    auto end = std::chrono::system_clock::now();
    auto ms = millis(end,start);
    auto usecPerOp = (ms*1000)/(double)nr;
    std::cout << "batch insert time " << nr << " records = " << ms << " ms, usec per ops " << usecPerOp  << "\n"; 
    start = std::chrono::system_clock::now();
    db->close();
    end = std::chrono::system_clock::now();
    ms = millis(end,start);
    std::cout << "close time " << ms << " ms\n";
    std::cout << "database size " << dbsize(dbname) << "\n";
}

static void _testCompact() {
    Options options(true);
    options.maxSegments=64;

    auto db = Database::open(dbname,options);
    auto start = std::chrono::system_clock::now();
    db->closeWithMerge(1);
    auto end = std::chrono::system_clock::now();
    auto ms = millis(end,start);
    std::cout << "compact time " << ms << " ms\n";
    std::cout << "database size " << dbsize(dbname) << "\n";
}

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

static void _testReadSequential(DatabaseRef &db) {

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
}

static void _testRead() {
    auto db = Database::open(dbname,Options());

    _testReadRandom(db);
    _testReadRandom(db);
    _testReadSequential(db);

    db->close();
}

BOOST_AUTO_TEST_CASE( dbbench ) {
    for(int i=0;i<vSize;i++) {
        value[i] = (uint8_t)rand();
    }
    _testWrite(false,true);
    _testWrite(true,true);
    _testBatch();
    _testWrite(false,false);
    _testRead();
    _testCompact();
    _testRead();
}
