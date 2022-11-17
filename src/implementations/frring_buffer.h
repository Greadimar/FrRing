#ifndef FRRING_BUFFER_H
#define FRRING_BUFFER_H
#include "frring_tools.h"
namespace FrRingDetails{
template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrRingBuffer
{
public:
    static_assert(is_power_of_two(bufSize), "please, provide size of buffer as power of 2");
    FrRingBuffer():
        writerState{new char[bufSize], 0},
        readerState{writerState.buffer, 0},
        head{0},
        tail{0}{
    };
    ~FrRingBuffer(){
        delete[] writerState.buffer;
    }
    //dequeue
    //enqueue_n
    //dequeue_n
    LocalState writerState;
    LocalState readerState;
    SharedState head;
    SharedState tail;
    alignas(CACHE_LINE_SIZE) int overlapMaskWrite{bufSize-1};
    alignas(CACHE_LINE_SIZE) int overlapMaskRead{bufSize-1}; // same
    StatMonitor writerMon;
    StatMonitor readerMon;
    // LocalState
};
}

#endif // FRRING_BUFFER_H
