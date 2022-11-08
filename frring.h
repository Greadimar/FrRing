#ifndef FRRING_H
#define FRRING_H

#include <cstddef>
#include <array>
#include <atomic>
#include <cstring>
#include <bit>
#define CACHE_LINE_SIZE 64 // 64 bytes cache line size for x86-64 processors // tosdfsd
constexpr int CIRCULAR_BUFFER_SIZE = 0x10000; // power of 2
struct alignas(CACHE_LINE_SIZE) LocalState
{
    char* buffer;
    int pos;
};
struct alignas(CACHE_LINE_SIZE) SharedState
{
    std::atomic_int pos;
};

//#define NUMBER_OF_SLOTS 2190 //2190 =  10 MB (Buffer Size) / 4.688 KB (Size of each buffer)
//#define MAX_CLAIM_ATTEMPTS 500000
constexpr bool is_power_of_two(int x)
{
    return x && ((x & (x-1)) == 0);
}

struct StatMonitor{
    std::atomic_int bufferFullCount{0};
    std::atomic_int overlaps{0};
};

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrRing
{
public:
    static_assert(is_power_of_two(bufSize), "please, provide size of buffer as power of 2");
    FrRing():
        writerState{new char[bufSize], 0},
        readerState(writerState),
        head{0},
        tail{0}{
    };

    //dequeue
    //enqueue_n
    //dequeue_n
    LocalState writerState;
    LocalState readerState;
    SharedState head;
    SharedState tail;
    alignas(CACHE_LINE_SIZE) int overlapMaskWrite{bufSize-1};
    alignas(CACHE_LINE_SIZE) int overlapMaskRead{bufSize-1}; // same
    StatMonitor monitor;
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
        if (size > free ){
            return false;
        }
        const int sizeBeforeBound = std::min(size, bufSize - tailpos);
        const int sizeRest = size - sizeBeforeBound;
        std::memcpy(_ring.writerState.buffer + _ring.writerState.pos, data, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
        std::memcpy(_ring.writerState.buffer, data + sizeBeforeBound, sizeRest);
        _ring.head.pos.store((_ring.writerState.pos + size) & _ring.overlapMaskWrite, std::memory_order_release);
        return true;
    }
    bool enqueue(char* data, int size){
        if (size > bufSize){ // unlikely
            return false;
        }
        int headpos = _ring.writerState.pos; // for mp should check
        int headUpdate = (headpos + size);
        if (headUpdate < bufSize){                       //no overlap
            int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
            if (headUpdate < tailpos){                                            // |xxxxH---HU--Txx|
                std::memcpy(_ring.writerState.buffer, data, size);
            }
            else{
                if (tailpos < headpos){                                           // |--TxxxH----HU--|
                    std::memcpy(_ring.writerState.buffer, data, size);
                }
                else{
                    _ring.monitor.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                    return false;                   //buffer is full             // |xxH----TxxHUxxx|
                }
            }
        }
        else if (headUpdate >= bufSize){                   //overlap unlikely
            _ring.monitor.overlaps.fetch_add(1, std::memory_order_relaxed);
            int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
            headUpdate &= _ring.overlapMaskWrite;
            if ((headUpdate) < tailpos){
                if (tailpos < headpos){                                           // |--HU---TxxxxH--|
                    int sizeBeforeBound = bufSize - size;
                    int sizeRest = size - sizeBeforeBound;
                    std::memcpy(_ring.writerState.buffer + headpos, data, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
                    std::memcpy(_ring.writerState.buffer, data + sizeBeforeBound, sizeRest);
                }
                else{
                    _ring.monitor.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                    return false;    //buffer is full                            // |xxHUxxxH-----Tx|
                }
            }
            else{
                _ring.monitor.bufferFullCount.fetch_add(1, std::memory_order_relaxed);
                return false;       //buffer is full                             // |--TxxHUxxxxxH--|
            }
        }
        _ring.writerState.pos = headUpdate;
        _ring.head.pos.store(headUpdate, std::memory_order_release);
    }
};

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrConsumer{
public:
    FrConsumer(FrRing<bufSize>& ring): _ring(ring){}
    FrRing<bufSize>& _ring;
    inline bool isAvailable(int size, int headpos, int tailpos){
        return size < ((headpos > tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos));
    }
    bool dequeue(char* data, int size){
        int headpos = _ring.head.pos.load(std::memory_order_acquire);
        const int& tailpos = _ring.readerState.pos;
        if (!isAvailable(size, headpos, tailpos)) return false;
        //        int available = (headpos > tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos);
        //        if (size > available ) return false;

        const int sizeBeforeBound = std::min(size, bufSize - headpos);
        const int sizeRest = size - sizeBeforeBound;
        std::memcpy(_ring.readerState.buffer + _ring.readerState.pos, data, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
        std::memcpy(_ring.readerState.buffer, data + sizeBeforeBound, sizeRest);
        _ring.tail.pos.store((_ring.readerState.pos + size) & _ring.overlapMaskRead);
        return false;
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
