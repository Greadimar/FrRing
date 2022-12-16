#ifndef IMPLEMENTATIONCHECKER_H
#define IMPLEMENTATIONCHECKER_H
#include "../src/frring.h"
#include "../src/implementations/frring_2memcp.h"
#include "../src/implementations/frring_ifbranches.h"
#include <algorithm>
#include <future>
using namespace std::chrono_literals;


struct CheckSettings{
    int bufSize{10000};
    int sizeToWrite{1500};
    int sizeToRead{1500};
    int maxTriesBeforeStuck{10000000};
};

class ImplementationChecker
{
public:
    ImplementationChecker(const CheckSettings& s): sets(s), inBuf(s.bufSize), outBuf(s.bufSize)
        {
        srand(time(0));
        std::generate(inBuf.begin(), inBuf.end(), [](){
            static char i = 0;
            return i++;
        });
    }
    CheckSettings sets;
    FrRing ring;
    std::vector<char> inBuf;
    std::vector<char> outBuf;
    struct Result{
        bool complete{false};
        std::chrono::system_clock::duration timeToComplete;
    };
    std::future<Result> prodFut;
    std::future<Result> consFut;
    bool checkInAndOut(int chunk);
    template <class TProdImpl, class TConsImpl>
    void test(){
        TProdImpl frProd(ring);
        TConsImpl frCons(ring);
    }
    template <class TProdImpl> void runProducerInThread(TProdImpl& producer){
        auto prod = [=, &producer](){
            bool complete{true};
            auto start = std::chrono::system_clock::now();
            for (int i = 0; i < sets.bufSize / sets.sizeToWrite; i++){
                int tries = 0; int maxTries = sets.maxTriesBeforeStuck;
                while (!producer.enqueue(inBuf.data() + i* sets.sizeToWrite, sets.sizeToWrite)){
                    //few tries - > sleep 1msec
                    tries++;
                    if (tries % (maxTries/4) == 0){
                       // std::cout <<  "sleep producer!" << std::endl;
                        std::this_thread::sleep_for(10ms);
                    }
                    if (tries >= maxTries){
                        std::cout << "enq buffer stuck" << std::endl;
                        complete = false;
                        break;
                    }
                }
            }
            auto end = std::chrono::system_clock::now();
            Result r{complete, end - start};
            return r;
        };
        prodFut = std::async(prod);
        //std::cout <<"prod completed in" << t3.count() << "msecs" <<"  " << std::this_thread::get_id() << std::endl;
    }
    template <class TConsImpl> void runConsumerInThread(TConsImpl& consumer){
        auto cons = [=, &consumer](){
            bool complete{true};
            auto start = std::chrono::system_clock::now();
            for (int i = 0; i < sets.bufSize / sets.sizeToRead; i++){
                int tries = 0; int maxTries = sets.maxTriesBeforeStuck;
                while (!consumer.dequeue(outBuf.data() + i* sets.sizeToRead, sets.sizeToRead)){
                    tries++;
                    if (tries % (maxTries/4) == 0){
                        //std::cout <<  "sleep consumer!" << std::endl;
                        std::this_thread::sleep_for(10ms);
                    }
                    if (tries >= maxTries){
                        std::cout << "deq buffer stuck" << std::endl;
                        complete = false;
                        break;
                    }
                }
            }
            auto end = std::chrono::system_clock::now();
            Result r{complete, end - start};
            return r;
        };
        consFut = std::async(cons);
       // std::cout <<"prod completed in" << t3.count() << "msecs" <<"  " << std::this_thread::get_id() << std::endl;
    }
};

#endif // IMPLEMENTATIONCHECKER_H
