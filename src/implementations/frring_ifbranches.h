#ifndef FRRING_IFBRANCHES_H
#define FRRING_IFBRANCHES_H
#include <cstddef>
#include <cstring>
#include "frring_buffer.h"
#include "frring_tools.h"

namespace FrRingImplementations{
using namespace FrRingDetails;

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrProducer_ifBranches{
public:
    FrProducer_ifBranches(FrRingBuffer<bufSize>& ring): _ring(ring){}
    FrRingBuffer<bufSize>& _ring;

    bool enqueue(char* data, int size){
        if (size > bufSize){ // unlikely
            return false;
        }
        int headpos = _ring.writerState.pos; // for mp should check
        int headUpdate = (headpos + size);
        int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
//        std::cout << "enq: t, h, hu bf: " << tailpos <<  " " << headpos <<  " " << headUpdate
//                  << " " << _ring.writerMon.bufferFullCount << " "  << _ring.writerMon.overlaps<< std::endl;
        if (headUpdate < bufSize){                       //no overlap
            if (headUpdate < tailpos){                                            // |xxxxH---HU--Txx|
                std::memcpy(_ring.writerState.buffer + headpos, data, size);
            }
            else{
                if (tailpos <= headpos){                                           // |--TxxxH----HU--|
                    std::memcpy(_ring.writerState.buffer + headpos, data, size);
                }
                else{
                    _ring.writerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                    return false;                   //buffer is full             // |xxH----TxxHUxxx|
                }
            }
        }
        else if (headUpdate >= bufSize){                   //overlap unlikely
            _ring.writerMon.overlaps.fetch_add(1, std::memory_order_relaxed);
            headUpdate &= _ring.overlapMaskWrite;
            if ((headUpdate) < tailpos){
                if (tailpos < headpos){                                           // |--HU---TxxxxH--|
                    int sizeBeforeBound = bufSize - headpos;
                    int sizeRest = size - sizeBeforeBound;
                    std::memcpy(_ring.writerState.buffer + headpos, data, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
                    std::memcpy(_ring.writerState.buffer, data + sizeBeforeBound, sizeRest);
                }
                else{
                    _ring.writerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                    return false;    //buffer is full                            // |xxHUxxxH-----Tx|
                }
            }
            else{
                _ring.writerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                return false;       //buffer is full                             // |--TxxHUxxxxxH--|
            }
        }
        _ring.writerState.pos = headUpdate;
        _ring.head.pos.store(headUpdate, std::memory_order_release);
        return true;
    }
};

}

#endif // FRRING_IFBRANCHES_H

