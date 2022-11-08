
#include <QApplication>
#include "frring_test.h"
int main(int argc, char *argv[])
{
    // QApplication a(argc, argv);
    FrRingTest test;

    for (int t = 0; t < 20; t++){
        test.runProducerInThread();
        test.runConsumerInThread();
        while (test.ready != 2){
        }
        test.ready = 0;
    }
    std::cout << "test.mon full" << test.ring.monitor.bufferFullCount  << std::endl;
    std::cout << "test.mon overl" << test.ring.monitor.overlaps  << std::endl;
    //return a.exec();
}
