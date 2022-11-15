#ifndef IMPLEMENTATIONTESTS_H
#define IMPLEMENTATIONTESTS_H

#endif // IMPLEMENTATIONTESTS_H
#include <gtest/gtest.h>
#include "implementationchecker.h"
#include <chrono>
#include <iostream>
#include <string>
using namespace std::chrono_literals;

namespace{
using namespace FrRingImplementations;

void doTest(const CheckSettings& sets, std::string name){

    using Prod = FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>;
    using Con = FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>;
    ImplementationChecker checker(sets);
    Prod prod(checker.ring);
    Con con(checker.ring);
    checker.runProducerInThread(prod);
    checker.runConsumerInThread(con);
    auto timeout = 5s;
    auto prodStatus = checker.prodRes.wait_for(timeout);
    auto consStatus = checker.consRes.wait_for(timeout);
    EXPECT_EQ(prodStatus, std::future_status::ready) << name << "producer failed time";
    EXPECT_EQ(consStatus, std::future_status::ready) << name << "consumer failed time";
    EXPECT_TRUE(checker.prodRes.get().complete) << name << "producer failed iterations";
    EXPECT_TRUE(checker.consRes.get().complete) << name << "consumer failed iterations";
    EXPECT_TRUE(checker.checkInAndOut()) << name << "failed result";

}

TEST(Implementation, twomemcp_def){
    doTest({}, "default");
   // EXPECT_FALSE(true);
}

}
