#ifndef IMPLEMENTATIONCHECKER_H
#define IMPLEMENTATIONCHECKER_H
#include "../frring.h"
#include "../src/frring_2memcp.h"
#include "../src/frring_ifbranches.h"
#include "../src/frring_2memcp.h"
#include <algorithm>
#include <future>


struct Settings{
    int bufSize{10000};
    int chunkSize{100};
    int sizeToWrite{1500};
    int sizeToRead{1500};
    int maxTriesBeforeStuck{100000};
};

class ImplementationChecker
{
public:

    ImplementationChecker(Settings s): sets(s), inBuf(s.bufSize), outBuf(s.bufSize)
        {
        srand(time(0));
        std::generate(inBuf.begin(), inBuf.end(), [](){
            static char i = 0;
            return i++;
        });
    }
    Settings sets;
    FrRing ring;
    std::vector<char> inBuf;
    std::vector<char> outBuf;

    struct Result{
        std::chrono::system_clock::duration timeToComplete;
    };
    std::future<Result> prodRes;
    std::future<Result> conRes;
    template <class TProdImpl, class TConsImpl>
    void test(){
        TProdImpl frProd(ring);
        TConsImpl frCons(ring);
    }
    template <class TProdImpl> void runProducerInThread(TProdImpl& producer){
        auto prod = [=](){
            auto start = std::chrono::system_clock::now();
            for (int i = 0; i < sets.bufSize / sets.chunkSize; i++){
                int tries = 0; int maxTries = 10000000;
                while (!producer.enqueue2(inBuf.data() + i* sets.chunkSize, sets.chunkSize)){
                    tries++;
                    if (tries >= maxTries){
                        std::cout << "enq buffer stuck" << std::endl;
                        break;
                    }
                }
            }
            auto end = std::chrono::system_clock::now();
            Result r;
            r.timeToComplete = end - start;
            return r;
        };
        prodRes = std::async(prod);
        std::cout <<"prod completed in" << t3.count() << "msecs" <<"  " << std::this_thread::get_id() << std::endl;
        th.detach();
    }
};

#endif // IMPLEMENTATIONCHECKER_H
