#ifndef IMPLEMENTATIONTESTS_H
#define IMPLEMENTATIONTESTS_H

#endif // IMPLEMENTATIONTESTS_H
#include <gtest/gtest.h>
#include "implementationchecker.h"
#include <chrono>
#include <iostream>
#include <string>

namespace{
using namespace FrRingImplementations;

template <class ProdImpl, class ConImpl>
void doTest(const CheckSettings& sets, std::string name){

    using Prod = ProdImpl;
    using Con = ConImpl;
    ImplementationChecker checker(sets);
    Prod prod(checker.ring);
    Con con(checker.ring);
    checker.runProducerInThread(prod);
    checker.runConsumerInThread(con);
    auto timeout = 30s;
    auto prodStatus = checker.prodFut.wait_for(timeout);
    auto consStatus = checker.consFut.wait_for(timeout);
    EXPECT_EQ(prodStatus, std::future_status::ready) << name << "producer failed time";
    EXPECT_EQ(consStatus, std::future_status::ready) << name << "consumer failed time";
    auto prodRes = checker.prodFut.get();
    auto conRes = checker.consFut.get();
    EXPECT_TRUE(prodRes.complete) << name << "producer failed iterations";
    EXPECT_TRUE(conRes.complete) << name << "consumer failed iterations";
    EXPECT_TRUE(checker.checkInAndOut(std::min(sets.sizeToRead, sets.sizeToWrite))) << name << " failed result";
    std::cout << name <<" complete in {p,c} :{" << prodRes.timeToComplete.count() << ", " <<
                 conRes.timeToComplete.count() << "}" << std::endl;
    std::cout << name <<" declinesFull, declinesEmpty: " << checker.ring.writerMon.declines << " " << checker.ring.readerMon.declines << std::endl;

}

CheckSettings sets{
    10000,
    100, 100, 100000
};
CheckSettings setsNotFoldChunks{
    10000,
    1500, 1500, 10000000
};
CheckSettings setsOverlap{
    4 * FrRingDetails::CIRCULAR_BUFFER_SIZE,
    1000, 1000, 10000000
};
CheckSettings setsMultipleOfBS{
    FrRingDetails::CIRCULAR_BUFFER_SIZE/0xf,
    FrRingDetails::CIRCULAR_BUFFER_SIZE/0xfff, FrRingDetails::CIRCULAR_BUFFER_SIZE/0xfff, 10000000
};
CheckSettings setsOverlapMultipleOfBs{
    4 * FrRingDetails::CIRCULAR_BUFFER_SIZE,
    FrRingDetails::CIRCULAR_BUFFER_SIZE/0xfff, FrRingDetails::CIRCULAR_BUFFER_SIZE/0xfff, 10000000
};
CheckSettings setsOverlapBigBySmall{
    100 * FrRingDetails::CIRCULAR_BUFFER_SIZE,
    1500, 1500, 10000000
};

TEST(TwoMem_impl, def){
    doTest<FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
          FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
            (sets, "2memcp");
}
TEST(TwoMem_impl, tsetsNotFoldChunks){
    doTest<FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
          FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
            (setsNotFoldChunks, "2memcp-setsNotFoldChunks");
}
TEST(TwoMem_impl, setsoverlap){
        doTest<FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsOverlap, "2memcp-setsoverlap");
}
TEST(TwoMem_impl, setsMultipleOfBS){
        doTest<FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsMultipleOfBS, "2memcp-setsMultipleOfBS");
}
TEST(TwoMem_impl, setsOverlapMultipleOfBs){
        doTest<FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsOverlapMultipleOfBs, "2memcp-setsOverlapMultipleOfBs");
}

TEST(TwoMem_impl, setsOverlapBigBySmall){
        doTest<FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsOverlapBigBySmall, "2memcp-setsOverlapBigBySmall");
}

TEST(IfBranch_impl, def){
    doTest<FrProducer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
          FrConsumer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
            (sets, "ifBranches");
}
TEST(IfBranch_impl, setsNotFoldChunks){
    doTest<FrProducer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
          FrConsumer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
            (setsNotFoldChunks, "ifBranches-setsNotFoldChunks");
}
TEST(IfBranch_impl, setsOverlap){
        doTest<FrProducer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsOverlap, "ifBranches-setsoverlap");
}
TEST(IfBranch_impl, setsMultipleOfBS){
        doTest<FrProducer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsMultipleOfBS, "ifBranches-setsMultipleOfBS");
}
TEST(IfBranch_impl, setsOverlapMultipleOfBs){
        doTest<FrProducer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsOverlapMultipleOfBs, "ifBranches-setsOverlapMultipleOfBs");
}


TEST(IfBranch_impl, setsOverlapBigBySmall){
    for (int i = 0; i < 100; i++){
        doTest<FrProducer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>,
              FrConsumer_ifBranches<FrRingDetails::CIRCULAR_BUFFER_SIZE>>
                (setsOverlapBigBySmall, "ifBranches-setsOverlapBigBySmall");
    }
}

}
