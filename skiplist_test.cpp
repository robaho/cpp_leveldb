#define BOOST_TEST_MODULE skiplist
#include <boost/test/included/unit_test.hpp>

#include <iostream>
#include <random>
#include <algorithm>

#include "skiplist.h"
#include "keyvalue.h"

static int compare(const int& a,const int& b) {
    return a-b;
}

BOOST_AUTO_TEST_CASE( skiplist ) {
    SkipList<int> s(compare);
    s.put(1);
    s.put(2);
    BOOST_TEST( s.contains(1));
    BOOST_TEST( s.contains(2));
    BOOST_TEST( !s.contains(3));
}

BOOST_AUTO_TEST_CASE( skiplist_2 ) {
    SkipList<int> s(compare);
    s.put(1);
    s.put(2);
    s.put(1);
    s.put(2);
    BOOST_TEST( s.contains(1));
    BOOST_TEST( s.contains(2));
    BOOST_TEST( !s.contains(3));
}

BOOST_AUTO_TEST_CASE( skiplist_remove ) {
    SkipList<int> s(compare);
    s.put(1);
    s.put(2);
    BOOST_TEST( s.contains(1));
    BOOST_TEST( s.contains(2));
    s.remove(2);
    BOOST_TEST( s.contains(1));
    BOOST_TEST( !s.contains(2));
    s.put(2);
    BOOST_TEST( s.contains(2));
}

int compareKey(const KeyValue& a,const KeyValue& b) {
    return a.key.compareTo(b.key);
}

BOOST_AUTO_TEST_CASE( skiplist_keyvalue ) {
    SkipList<const KeyValue> s(compareKey);
    s.put(KeyValue("1","value1"));
    s.put(KeyValue("2","value2"));
    BOOST_TEST( s.contains(Key("1")));
    BOOST_TEST( s.contains(Key("2")));
    BOOST_TEST( !s.contains(Key("3")));
    s.remove(Key("2"));
    BOOST_TEST( !s.contains(Key("2")));
}

BOOST_AUTO_TEST_CASE( skiplist_keyvalue_get ) {
    SkipList<const KeyValue> s(compareKey);
    s.put(KeyValue("1","value1"));
    s.put(KeyValue("2","value2"));
    bool result = s.get(KeyValue("1","")) == KeyValue("1","value1");
    BOOST_TEST( result );
    result = s.get(KeyValue("1","")) == KeyValue("1","value2");
    BOOST_TEST( !result );
}

BOOST_AUTO_TEST_CASE( skiplist_keyvalue2 ) {
    std::vector<int> vals;
    for(int i=0;i<100;i++) vals.push_back(i);

    auto rng = std::default_random_engine {};
    std::shuffle(std::begin(vals), std::end(vals), rng);

    SkipList<const KeyValue> s(compareKey);
    for(auto val : vals) {
        s.put(KeyValue(""+std::to_string(val),"value"+std::to_string(val)));
    }
    for(int i=0;i<100;i++) {
        BOOST_TEST(s.contains(KeyValue(""+std::to_string(i),"")),"i is " << i);
    }
}

BOOST_AUTO_TEST_CASE( skiplist_iterator ) {
    SkipList<const KeyValue> s(compareKey);

    BOOST_TEST(!s.iterator().valid());
    BOOST_TEST(!s.iterator().seekToFirst());
    s.put(KeyValue("1","value1"));
    BOOST_TEST(s.iterator().seekToFirst());
    s.put(KeyValue("2","value2"));
    s.put(KeyValue("3","value3"));
    s.put(KeyValue("4","value4"));
    s.put(KeyValue("5","value5"));
    s.put(KeyValue("6","value6"));
    auto itr = s.iterator();
    itr.seek(KeyValue("3",""));
    BOOST_TEST(itr.key()==KeyValue("3","value3"));
    BOOST_TEST(itr.next());
    BOOST_TEST(itr.key()==KeyValue("4","value4"));
    BOOST_TEST(itr.next());
    BOOST_TEST(itr.next());
    BOOST_TEST(!itr.next());
}


