#define BOOST_TEST_MODULE logfile
#include <boost/test/included/unit_test.hpp>

#include <filesystem>
#include <exception>

#include "logfile.h"

namespace fs = std::filesystem;

static void writeLogFile(const Options options) {
    fs::remove("test/log.0");
    fs::create_directory("test");

    auto lf = LogFile("test",0,options);
    lf.write("mykey","myvalue");
    lf.startBatch(2);
    lf.write("batchkey1","batchvalue1");
    lf.write("batchkey2","batchvalue2");
    lf.endBatch(2);
    lf.close();
}

static bool testKeyValue(SkipList<const KeyValue> &list,const ByteBuffer& key,const ByteBuffer& value) {
    auto kv = list.get(KeyValue(key,value));
    return kv.value==value;
}

BOOST_AUTO_TEST_CASE( logfile ) {
    Options options;
    writeLogFile(options);
    SkipList<const KeyValue> list(keyValueCompare(options));
    readLogFile(list,"test/log.0",options);
    BOOST_TEST(testKeyValue(list,"mykey","myvalue"));
    BOOST_TEST(testKeyValue(list,"batchkey1","batchvalue1"));
    BOOST_TEST(testKeyValue(list,"batchkey2","batchvalue2"));
}

BOOST_AUTO_TEST_CASE( logfile_sync ) {
    Options options;
    options.enableSyncWrite=true;
    writeLogFile(options);
    SkipList<const KeyValue> list(keyValueCompare(options));
    readLogFile(list,"test/log.0",options);
    BOOST_TEST(testKeyValue(list,"mykey","myvalue"));
    BOOST_TEST(testKeyValue(list,"batchkey1","batchvalue1"));
    BOOST_TEST(testKeyValue(list,"batchkey2","batchvalue2"));
}


BOOST_AUTO_TEST_CASE( logfile_invalidfile ) {
    Options options;
    writeLogFile(options);
    truncate("test/log.0",1);
    options.batchReadMode = Options::BatchReadMode::returnOpenError;
    SkipList<const KeyValue> list(keyValueCompare(options));
    BOOST_REQUIRE_THROW(readLogFile(list,"test/log.0",options),std::exception);
}

BOOST_AUTO_TEST_CASE( logfile_partialbatch ) {
    Options options;
    writeLogFile(options);
    truncate("test/log.0",84-12); // truncate into batch
    options.batchReadMode = Options::BatchReadMode::returnOpenError;
    {
        SkipList<const KeyValue> list(keyValueCompare(options));
        BOOST_REQUIRE_THROW(readLogFile(list,"test/log.0",options),std::exception);
    }
    options.batchReadMode = Options::BatchReadMode::discardPartial;
    {
        SkipList<const KeyValue> list(keyValueCompare(options));
        readLogFile(list,"test/log.0",options);
        BOOST_TEST(testKeyValue(list,"mykey","myvalue"));
        BOOST_TEST(!testKeyValue(list,"batchkey1","batchvalue1"));
        BOOST_TEST(!testKeyValue(list,"batchkey2","batchvalue2"));
    }
    options.batchReadMode = Options::BatchReadMode::applyPartial;
    {
        SkipList<const KeyValue> list(keyValueCompare(options));
        readLogFile(list,"test/log.0",options);
        BOOST_TEST(testKeyValue(list,"mykey","myvalue"));
        BOOST_TEST(testKeyValue(list,"batchkey1","batchvalue1"));
        BOOST_TEST(!testKeyValue(list,"batchkey2","batchvalue2"));
    }
}


