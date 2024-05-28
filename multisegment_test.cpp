#define BOOST_TEST_MODULE multisegment
#include <boost/test/included/unit_test.hpp>
#include <iostream>
#include <random>
#include <algorithm>

#include "skiplist.h"
#include "keyvalue.h"
#include "memorysegment.h"
#include "multisegment.h"
#include "segment.h"

BOOST_AUTO_TEST_CASE( multisegment ) {
    auto m1 = MemorySegment::newMemoryOnlySegment();

    for (int i= 0; i < 1; i++) {
		m1->put("mykey" + std::to_string(i), "myvalue" + std::to_string(i));
	}
	auto m2 = MemorySegment::newMemoryOnlySegment();
	for (int i = 1; i < 2; i++) {
		m2->put("mykey" + std::to_string(i), "myvalue" + std::to_string(i));
	}
    auto ms = MultiSegment::newMultiSegment({m1,m2});
	auto itr = ms->lookup("","");
	int count = 0;
	while(true) {
		auto kv = itr->next();
		if(kv.key.empty()) {
			break;
		}
		count++;
	}
    BOOST_TEST(count == 2);
}

BOOST_AUTO_TEST_CASE( multisegment_edit ) {
    auto m1 = MemorySegment::newMemoryOnlySegment();

    for (int i= 0; i < 1; i++) {
		m1->put("mykey" + std::to_string(i), "myvalue" + std::to_string(i));
	}
	auto m2 = MemorySegment::newMemoryOnlySegment();
	for (int i = 0; i < 1; i++) {
		m2->put("mykey" + std::to_string(i), "myvaluex" + std::to_string(i));
	}
    auto ms = MultiSegment::newMultiSegment({m1,m2});
	auto itr = ms->lookup("","");
    auto kv = itr->next();
    BOOST_TEST(kv.value=="myvaluex0");
    auto kv2 = itr->next();
    BOOST_TEST(kv2.key.empty());
}

BOOST_AUTO_TEST_CASE( multisegment_remove ) {
    auto m1 = MemorySegment::newMemoryOnlySegment();
    for (int i= 0; i < 1; i++) {
		m1->put("mykey" + std::to_string(i), "myvalue" + std::to_string(i));
	}
	auto m2 = MemorySegment::newMemoryOnlySegment();
	for (int i = 0; i < 1; i++) {
		m2->put("mykey" + std::to_string(i), "");
	}
    auto ms = MultiSegment::newMultiSegment({m1,m2});
	auto itr = ms->lookup("","");
    int count = 0;
    while(!(itr->next().value.empty())) count++;
    BOOST_TEST(count==0);
}


