#ifndef FRRING_2MEMCP_H
#define FRRING_2MEMCP_H
#include <array>
#include <cstring>
#include "frring_buffer.h"
#include "frring_tools.h"



namespace FrRingImplementations{
using namespace FrRingDetails;
template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrProducer_2memcp{
public:
    FrProducer_2memcp(FrRingBuffer<bufSize>& ring): _ring(ring){}
    FrRingBuffer<bufSize>& _ring;
    bool enqueue(char* data, int size){
        int tailpos = _ring.tail.pos.load(std::memory_order_acquire);
        const int& headpos = _ring.writerState.pos;
        int free = (tailpos > headpos)? (tailpos - headpos) : (bufSize - headpos + tailpos);
        //std::cout << "enq: t, h, s: " << tailpos <<  " " << headpos <<  " " << size << std::endl;
                  //<< " " << _ring.writerMon.bufferFullCount << " "  << _ring.writerMon.overlaps<< std::endl;
        if (size > free ){
           // std::cout << "full enq s, f " << size <<  " " << free << std::endl;
            _ring.writerMon.declines.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        const int sizeBeforeBound = std::min(size, bufSize - headpos);
        const int sizeRest = size - sizeBeforeBound;
        if (sizeRest > 0){
            //std::cout << "enq overlap " << sizeRest << std::endl;
            _ring.writerMon.overlaps.fetch_add(1, std::memory_order_relaxed);
        }
        std::memcpy(_ring.writerState.buffer + _ring.writerState.pos, data, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
        std::memcpy(_ring.writerState.buffer, data + sizeBeforeBound, sizeRest);
        _ring.writerState.pos = (_ring.writerState.pos + size ) & _ring.overlapMaskWrite;
        _ring.head.pos.store(_ring.writerState.pos, std::memory_order_release);
        return true;
    }

};

template<int bufSize = CIRCULAR_BUFFER_SIZE>
class FrConsumer_2memcp{
public:
    FrConsumer_2memcp(FrRingBuffer<bufSize>& ring): _ring(ring){}
    FrRingBuffer<bufSize>& _ring;
    inline bool isAvailable(int size, int headpos, int tailpos){ // 100, 10000, 9900 // false? should be true
        return size <= ((headpos >= tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos));
    }
    bool dequeue(char* data, int size){
        int headpos = _ring.head.pos.load(std::memory_order_acquire);
        const int& tailpos = _ring.readerState.pos;
       // std::cout << "deq: t, h, s: " << tailpos <<  " " << headpos <<  " " << size << std::endl;
        if (!isAvailable(size, headpos, tailpos)){
         //   std::cout << "deq s, is empty " << " t, h, s: " << tailpos <<  " " << headpos <<  " " << size << std::endl;
            _ring.readerMon.declines.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        //        int available = (headpos > tailpos)? (headpos - tailpos) : (bufSize - tailpos + headpos);
        //        if (size > available ) return false;

        const int sizeBeforeBound = std::min(size, bufSize - tailpos);
        const int sizeRest = size - sizeBeforeBound;
        if (sizeRest > 0){
            //std::cout << "deq overlap " << sizeRest << std::endl;
            _ring.readerMon.overlaps.fetch_add(1, std::memory_order_relaxed);
        }
        std::memcpy(data, _ring.readerState.buffer + _ring.readerState.pos, sizeBeforeBound);            //to optimize it: https://www.geeksforgeeks.org/write-memcpy/;
        std::memcpy(data + sizeBeforeBound, _ring.readerState.buffer, sizeRest);
        _ring.readerState.pos = (_ring.readerState.pos + size) & _ring.overlapMaskRead;
        _ring.tail.pos.store(_ring.readerState.pos, std::memory_order_release);
        return true;
    }
};
}





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

#endif // FRRING_2MEMCP_H
