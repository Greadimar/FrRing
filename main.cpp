
#include <QApplication>
#include "frring_test.h"
int main(int argc, char *argv[])
{
    // QApplication a(argc, argv);
    FrRingTest test;

    for (int t = 0; t < 1; t++){
        test.runProducerInThread();
        test.runConsumerInThread();
        while (test.ready != 2){
        }
        test.ready = 0;
    }
    std::cout << "test.mon full w r : " << test.ring.writerMon.bufferFullCount  <<" " <<test.ring.readerMon.bufferFullCount <<  std::endl;
    std::cout << "test.mon overl w r : "  << test.ring.writerMon.overlaps  <<" " <<test.ring.readerMon.overlaps <<  std::endl;
    std::cout << "comp: " << std::equal(test.inBuf.begin(), test.inBuf.end(), test.outBuf.begin(), test.outBuf.end()) <<std::endl;
    //return a.exec();
}
