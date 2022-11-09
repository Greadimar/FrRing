#ifndef FRRING_TEST_H
#define FRRING_TEST_H
#include "frring.h"
#include <thread>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <future>
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
    const int bufSize{10000}; //100mb;
    const int chunkSize{100};
    std::atomic_int ready{0};
    FrProducer<size> producer{ring};
    FrConsumer<size> consumer{ring};
    const int sizeToWrite{1500};
    const int sizeToRead{1500};
    void runProducerInThread(){
        std::thread th([=](){
            auto t = std::chrono::system_clock::now();
            for (int i = 0; i < bufSize / chunkSize; i++){
                int tries = 0; int maxTries = 10000000;
                while (!producer.enqueue2(inBuf.data() + i* chunkSize, chunkSize)){
                    tries++;
                    if (tries >= maxTries){
                        std::cout << "enq buffer stuck" << std::endl;
                        break;
                    }
                }
            }
            auto t2 = std::chrono::system_clock::now();
            auto t3 = t2-t;
            std::cout <<"prod completed in" << t3.count() << "msecs" <<"  " << std::this_thread::get_id() << std::endl;
            ready++;
        });
        th.detach();
        //        auto v = std::async([=](){
        //            for (int i = 0; i < bufSize / chunkSize; i++){
        //                producer.enqueue2(inBuf.data() + i* chunkSize, chunkSize);
        //            };
        //        });
    }
    void runConsumerInThread(){
        std::thread th([=](){
            auto t = std::chrono::system_clock::now();
            for (int i = 0; i < bufSize / chunkSize; i++){
                int tries = 0; int maxTries = 10000000;
                while (!consumer.dequeue2(outBuf.data() + i*chunkSize, chunkSize)){
                    tries++;
                    if (tries >= maxTries){
                        std::cout << "deq buffer stuck" << std::endl;
                        break;
                    }

                }
            }
            auto t2 = std::chrono::system_clock::now();
            auto t3 = t2-t;
            std::cout <<"cons completed in" << t3.count() << "msecs" <<"  " << std::this_thread::get_id()  << std::endl;
            ready++;
        });
        th.detach();
        //        auto v = std::async([=](){
        //            for (int i = 0; i < bufSize / chunkSize; i++){
        //                consumer.dequeue2(outBuf.data() + i* chunkSize, chunkSize);
        //            };
        //        });
    }
};

#endif // FRRING_TEST_H
