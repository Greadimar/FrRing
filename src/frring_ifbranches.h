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
    FrProducer_ifBranches(FrRingBuf<bufSize>& ring): _ring(ring){}
    FrRingBuf<bufSize>& _ring;

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

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrConsumer_2memcp{
public:
    FrConsumer_2memcp(FrRingBuf<bufSize>& ring): _ring(ring){}
    FrRingBuf<bufSize>& _ring;
    inline bool isAvailable(int size, int headpos, int tailpos){ // 100, 10000, 9900 // false? should be true
        return size <= ((headpos >= tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos));
    }

    bool dequeue(char* data, int size){
        if (size > bufSize){ // unlikely
            return false;
        }
        int tailPos = _ring.readerState.pos; // for mp should check
        int tailUpdate = (tailPos + size);
        int headpos = _ring.head.pos.load(std::memory_order_acquire);
//        std::cout << "deq: t, tu, h: " << tailPos << " " << tailUpdate <<   " " << headpos
//                  << " " << _ring.readerMon.bufferFullCount << " "  << _ring.readerMon.overlaps<< std::endl;
        if (tailUpdate < bufSize){                       //no overlap
            if (tailUpdate < headpos){                                                   // |---TxxxxTUxxxH--|
                std::memcpy(data, _ring.readerState.buffer + tailPos, size);
            }
            else{
                if (headpos < tailPos){                                                 // |xxH---TxxxxTUx|
                    std::memcpy(data, _ring.readerState.buffer + tailPos, size);
                }
                else{
                    _ring.readerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                    return false;                       //buffer is empty                //|---TxxxxH--TU-|
                }
            }
        }
        else if (tailUpdate >= bufSize){                   //overlap unlikely
            _ring.readerMon.overlaps.fetch_add(1, std::memory_order_relaxed);
            //int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
            tailUpdate &= _ring.overlapMaskRead;
            if ((tailUpdate) < headpos){
                if (headpos < tailPos){                                           // |xxHUxxH---Txx|
                    int sizeBeforeBound = bufSize - tailPos;
                    int sizeRest = size - sizeBeforeBound;
                    std::memcpy(data, _ring.readerState.buffer + tailPos, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
                    std::memcpy(data + sizeBeforeBound, _ring.readerState.buffer, sizeRest);
                }
                else{
                    _ring.readerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                    return false;    //buffer is empty                            // |--HU---TxxxH--|
                }
            }
            else{
                _ring.readerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                return false;       //buffer is full                             // |xxH--TU---Txxx|
            }
        }
        _ring.readerState.pos = tailUpdate;
        _ring.tail.pos.store(tailUpdate, std::memory_order_release);
        return true;
    }
};
}

#endif // FRRING_IFBRANCHES_H

