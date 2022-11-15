#ifndef FRRING_H
#define FRRING_H
#include "implementations/frring_2memcp.h"

using FrRing = FrRingDetails::FrRingBuffer<FrRingDetails::CIRCULAR_BUFFER_SIZE>;
using FrProducer = FrRingImplementations::FrProducer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>;
using FrConCumer = FrRingImplementations::FrConsumer_2memcp<FrRingDetails::CIRCULAR_BUFFER_SIZE>;

#endif // FRRING_H
