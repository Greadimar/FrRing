#ifndef FRRING_TEST_H
#define FRRING_TEST_H
#include "frring.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
class FrRingTest
{
public:
    FrRingTest(){
        srand(time(0));
        inBuf.resize(bufSize);
        outBuf.resize(bufSize);
        std::generate(inBuf.begin(), inBuf.end(), [](){
            static char i = 0;
            return i++;
        });
    };
    constexpr static int size = CIRCULAR_BUFFER_SIZE;
    std::vector<char> inBuf;
    std::vector<char> outBuf;
    FrRing<size> ring;
    const int bufSize{100000}; //100mb;
    const int chunkSize{100};
    std::atomic_int ready{0};
    FrProducer<size> producer{ring};
    FrConsumer<size> consumer{ring};
    const int sizeToWrite{1500};
    const int sizeToRead{1500};
    void runProducerInThread(){
        std::thread th([=](){
            auto t = std::chrono::system_clock::now();
            for (int i = 0; i < bufSize / chunkSize; i++)
                producer.enqueue(inBuf.data() + i* chunkSize, chunkSize);
            auto t2 = std::chrono::system_clock::now();
            auto t3 = t2-t;
            std::cout <<"prod completed in" << t3.count() << "msecs" << std::endl;
            ready++;
        });
        th.detach();
    };
    void runConsumerInThread(){
        std::thread th([=](){
             auto t = std::chrono::system_clock::now();
            for (int i = 0; i < bufSize / chunkSize; i++)
                consumer.dequeue(outBuf.data() + i*chunkSize, chunkSize);
            auto t2 = std::chrono::system_clock::now();
            auto t3 = t2-t;
            std::cout <<"cons completed in" << t3.count() << "msecs" << std::endl;
            ready++;
        });
        th.detach();
    }
};

#endif // FRRING_TEST_H
