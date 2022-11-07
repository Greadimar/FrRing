#ifndef FRRING_TEST_H
#define FRRING_TEST_H
#include "frring.h"

class frring_test
{
public:
    frring_test();
    constexpr static int size = CIRCULAR_BUFFER_SIZE;
    FrRing<size> ring;
    FrProducer<size> producer{ring};
    FrConsumer<size> consumer{ring};
    void runProducerInThread(){
        producer.enqueue()
    };
};

#endif // FRRING_TEST_H
