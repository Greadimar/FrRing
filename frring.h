#ifndef FRRING_H
#define FRRING_H

#include <cstddef>
#include <array>
#include <atomic>
#include <cstring>
#include <bit>
#include <iostream>
#define CACHE_LINE_SIZE 64 // 64 bytes cache line size for x86-64 processors // tosdfsd
constexpr int CIRCULAR_BUFFER_SIZE = 0x1000000; // power of 2
struct alignas(CACHE_LINE_SIZE) LocalState
{
    char* buffer;
    int pos;
};
struct alignas(CACHE_LINE_SIZE) SharedState
{
    std::atomic_int pos;
};


constexpr bool is_power_of_two(int x)
{
    return x && ((x & (x-1)) == 0);
}

struct alignas(CACHE_LINE_SIZE) StatMonitor{
    std::atomic_int declines{0};        //buffer is full (can't write), or buffer is empty(can't read)
    std::atomic_int overlaps{0};
};

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrRing
{
public:
    static_assert(is_power_of_two(bufSize), "please, provide size of buffer as power of 2");
    FrRing():
        writerState{new char[bufSize], 0},
        readerState{writerState.buffer, 0},
        head{0},
        tail{0}{
    };
    ~FrRing(){
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

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrProducer{
public:
    FrProducer(FrRing<bufSize>& ring): _ring(ring){}
    FrRing<bufSize>& _ring;
    bool enqueue2(char* data, int size){
        int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
        const int& headpos = _ring.writerState.pos;
        int free = (tailpos > headpos)? (tailpos - headpos) : (bufSize - headpos + tailpos);
        //std::cout << "enq: t, h, s: " << tailpos <<  " " << headpos <<  " " << size << std::endl;
                  //<< " " << _ring.writerMon.bufferFullCount << " "  << _ring.writerMon.overlaps<< std::endl;
        if (size > free ){
            std::cout << "full enq s, f " << size <<  " " << free << std::endl;
            _ring.writerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        const int sizeBeforeBound = std::min(size, bufSize - headpos);
        const int sizeRest = size - sizeBeforeBound;
        if (sizeRest > 0){
           // std::cout << "enq overlap " << sizeRest << std::endl;
            _ring.writerMon.overlaps.fetch_add(1, std::memory_order_relaxed);
        }
        std::memcpy(_ring.writerState.buffer + _ring.writerState.pos, data, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
        std::memcpy(_ring.writerState.buffer, data + sizeBeforeBound, sizeRest);
        _ring.writerState.pos = (_ring.writerState.pos + size ) & _ring.overlapMaskWrite;
        _ring.head.pos.store(_ring.writerState.pos , std::memory_order_release);
        return true;
    }
    bool enqueue(char* data, int size){
        if (size > bufSize){ // unlikely
            return false;
        }
        int headpos = _ring.writerState.pos; // for mp should check
        int headUpdate = (headpos + size);
        int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
        std::cout << "enq: t, h, hu bf: " << tailpos <<  " " << headpos <<  " " << headUpdate
                  << " " << _ring.writerMon.bufferFullCount << " "  << _ring.writerMon.overlaps<< std::endl;
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
            //int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
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
class FrConsumer{
public:
    FrConsumer(FrRing<bufSize>& ring): _ring(ring){}
    FrRing<bufSize>& _ring;
    inline bool isAvailable(int size, int headpos, int tailpos){ // 100, 10000, 9900 // false? should be true
        return size <= ((headpos >= tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos));
    }
    bool dequeue2(char* data, int size){
        int headpos = _ring.head.pos.load(std::memory_order_acquire);
        const int& tailpos = _ring.readerState.pos;
        //std::cout << "deq: t, h, s: " << tailpos <<  " " << headpos <<  " " << size << std::endl;
        if (!isAvailable(size, headpos, tailpos)){
            std::cout << "deq s, is empty " << " t, h, s: " << tailpos <<  " " << headpos <<  " " << size << std::endl;
             _ring.readerMon.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        //        int available = (headpos > tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos);
        //        if (size > available ) return false;

        const int sizeBeforeBound = std::min(size, bufSize - tailpos);
        const int sizeRest = size - sizeBeforeBound;
        if (sizeRest > 0){
           // std::cout << "deq overlap " << sizeRest << std::endl;
            _ring.readerMon.overlaps.fetch_add(1, std::memory_order_relaxed);
        }
        std::memcpy(data, _ring.readerState.buffer + _ring.readerState.pos, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
        std::memcpy(data + sizeBeforeBound, _ring.readerState.buffer, sizeRest);
        _ring.readerState.pos = (_ring.readerState.pos + size) & _ring.overlapMaskRead;
        _ring.tail.pos.store(_ring.readerState.pos);
        return true;
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
//constexpr int ethHeaderSize = 14;
//class CustomConsumer : public FrConsumer<CIRCULAR_BUFFER_SIZE>{
//public:

//    CustomConsumer(FrRing<CIRCULAR_BUFFER_SIZE>& ring): FrConsumer<CIRCULAR_BUFFER_SIZE>{ring}{

//    }
//    static inline int getSizeFromEthType(uint16_t ethType){
//        char low = ethType & 0xff;
//        char high = (ethType & 0xff00) >> 8;
//        return (static_cast<uint16_t>(low) << 8) | high;
//    }
//    bool tryConsumeMsg(char* outData){
//        int headpos = _ring.head.pos.load(std::memory_order_acquire);
//        const int& tailpos = _ring.readerState.pos;
//        if (!isAvailable(ethHeaderSize, headpos, tailpos)){
//            return false;
//        }
//        short ethType;
//        std::memcpy(&ethType, outData + 12, sizeof(short));
//        int sizeToRead = getSizeFromEthType(ethType);

//        if (!isAvailable(ethHeaderSize + sizeToRead, headpos, tailpos)){

//        }
//    };
//    bool processMsg(const MacHeader& mh, const BaseHeader& bh, char* data, int size){

//    }

//};

#endif // FRRING_H
