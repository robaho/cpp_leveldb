#define BOOST_TEST_MODULE database
#include <boost/test/included/unit_test.hpp>

#include <filesystem>

#include "database.h"
#include "exceptions.h"

namespace fs = std::filesystem;

static Options options(true);

BOOST_AUTO_TEST_CASE( database ) {
    try { Database::remove("test/mydb"); } catch(DatabaseNotFound){}

    auto db = Database::open("test/mydb",options);
    db->put("mykey","myvalue");
    BOOST_TEST(db->get("mykey")=="myvalue");
    BOOST_TEST(db->get("mykey2").empty());

    std::string largekey;
    for(int i=0;i<1000;i++) largekey = largekey + "somekey";

    BOOST_CHECK_THROW(db->put(largekey,"somevalue"),IllegalState);

    db->close();

    BOOST_CHECK_THROW(db->put("mykey","somevalue"),DatabaseClosed);
}

BOOST_AUTO_TEST_CASE( database_valid ) {
    try { Database::remove("test/mydb"); } catch(DatabaseNotFound){}

    auto db = Database::open("test/mydb",options);
    db->put("mykey","myvalue");
    db->close();

    fs::create_directory("test/mydb/somedir");
    // open should fail if directory has unexpected files
    BOOST_CHECK_THROW(Database::open("test/mydb",options),InvalidDatabase);
    fs::remove_all("test/mydb/somedir");
}


BOOST_AUTO_TEST_CASE( database_merge) {
    try { Database::remove("test/mydb"); } catch(DatabaseNotFound){}
    
    int nsegs = 100;

    Options options(true);
    options.disableAutoMerge = true;
    
    for(int i=0;i < nsegs; i++) {
        auto db = Database::open("test/mydb",options);
        for(int j=0;j<100;j++) {
            db->put("mykey"+std::to_string(j),"myvalue"+std::to_string(j));
        }
        db->closeWithMerge(0);
    }

    auto countFiles = []() {
        auto files = fs::directory_iterator("test/mydb");
        int count=0;
        for(auto f : files) {
            auto filename = f.path().filename().string();
            if(filename.starts_with("keys.") || filename.starts_with("data.")) {
                count++;
            }
        }
        return count;
    };

    BOOST_TEST(countFiles()==nsegs*2);
    auto db = Database::open("test/mydb",options);
    db->closeWithMerge(1);
    BOOST_TEST(countFiles()==2);

    db = Database::open("test/mydb",options);
    auto itr = db->lookup("","");
    int count=0;
    while(!(itr->next().key.empty())) count++;
    BOOST_TEST(count==100);
}