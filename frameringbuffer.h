#ifndef FRAMERINGBUFFER_H
#define FRAMERINGBUFFER_H
#include <QVector>
#include <net/ethernet.h>
#include <array>
#include <vector>
#include <memory>
#include <QThread>
#include <qbytearray.h>
#include <cstring>
//#include <glib-2.0/glib.h>
using EtherPayload = char[ETH_DATA_LEN];
struct Packet{
    struct Header{
        unsigned int destination;
        unsigned int source;
        unsigned short type;
        unsigned short tag;
        unsigned short n;   //current n of frame
        unsigned short N;   //count of frames in packet
    };
    Header h;
    char data[ETH_DATA_LEN - sizeof (Header)];
};
struct RawMsg{
    struct Header{
        Header(){};
        Header(const Packet::Header& ph): destination(ph.destination), source(ph.source), type(ph.type), tag(ph.tag){}
        unsigned int destination;
        unsigned int source;
        unsigned short type;
        unsigned short tag;
    };
    Header h;
    QByteArray msgBa;
};

struct EPacketCell{
    ether_header header;
    Packet packet;
    EPacketCell& operator = (const EPacketCell& other){
        header = other.header;
        packet = other.packet;
        return *this;
    }
    EPacketCell& operator = (EPacketCell&& other){
        header = std::move(other.header);
        packet = std::move(other.packet);
        return *this;
    }
};
constexpr int FRAME_BUFFERSIZE_MB = 64;
constexpr int FRAME_BUFFER_SIZE = FRAME_BUFFERSIZE_MB * 1024 * 1024 / sizeof (EPacketCell);
constexpr int MAX_FRAMES_IN_MSG = 300;

constexpr int COMPARE_HEADER_KEY_SIZE = sizeof (Packet::Header::destination) + sizeof (Packet::Header::source) + sizeof (Packet::Header::type);

//-------<tail>xxxxxxxxxxxx<head>-------

template <typename T, int size>
class FrameRingBuffer
{
public:
    explicit FrameRingBuffer(){
    };
    const int consumerCount{3};
    //    std::vector<std::shared_ptr<Consumer<T>>> consumers;
    //    std::vector<std::shared_ptr<Consumer<T>>> producers;
    std::array<T, size> buffer;
    std::atomic_int lastHead{0};    //writer
    std::atomic_int head{0};
    //  std::atomic_int lastUsed{0};    //
    std::atomic_int ready{0};
    std::atomic_int lastReady{0};

    std::atomic_int tail{0};    //reader
    std::atomic_int lastTail{0};    //reader
    //bool headOverlapped{false};
    //std::atomic_bool tailOverlapped{false};
    std::atomic_bool isRunning{false};
};
using FrameRing = FrameRingBuffer<EPacketCell, FRAME_BUFFER_SIZE>;


//class PcapProducer: public QThread{ //only once producer
//public:
//    PcapProducer(FrameRing& ring): m_ring(ring){}
//    std::atomic_int pr;
//    void run(){

//        while(m_ring.isRunning.load(std::memory_order_relaxed)){
//            tail = m_ring.lastTail.load(std::memory_order_relaxed);
//            head = m_ring.lastHead.load(std::memory_order_acquire); // just reading lastHead

//             //case 1: -------TxxxxxxxxRxxxxxH-----




//        }
//    }

//private:
//    FrameRing& m_ring;
//    int tail, head;
//    void moveHead();
//};

class Consumer: public QThread{
    Consumer(FrameRing& ring): m_ring(ring){}
    void run(){
        while(m_ring.isRunning.load(std::memory_order_relaxed)){
            QThread::sleep(0);
            do {
                ready = m_ring.ready.load(std::memory_order_relaxed);
                head = m_ring.lastHead.load(std::memory_order_acquire); // just reading lastHead
                readyNext = tail == FRAME_BUFFER_SIZE -1 ? 0: ready + 1;

                if (ready < tail) ready = tail; // just for the case
            }
            while (m_ring.ready.compare_exchange_weak(ready, readyNext));   //get next free ready cell


            //case 1: -------TxxxxxxxxRxxxxxH-----
            if (!checkMsgReady(m_ring.buffer[ready].packet)) continue; // if it's not ready continue running
            if(tail <= ready && ready < head){
                collectMsg();
            }
            // case 2: xRxxxxH-------------Txxxxxx
            else{
                collectMsgWithOverlap();
            }
            while (m_ring.lastReady != ready){    // wait
                 QThread::sleep(0);
            }
            m_ring.lastReady.store(readyNext, std::memory_order_relaxed);
            //trying to move Tail forward
            if (head < tail){
                tailNext = (FRAME_BUFFER_SIZE - MAX_FRAMES_IN_MSG) + head;
            }
            else{
                tailNext = std::min(0, head - FRAME_BUFFER_SIZE);
            }
            while (m_ring.lastTail != tail){    // wait
                 QThread::sleep(0);
            }
            m_ring.lastTail.store(tailNext, std::memory_order_relaxed);

        }
    }

public:

    void setCbProcess(const std::function<void (RawMsg &&)> &newCbProcess);

private:
    FrameRing& m_ring;
    QByteArray collectedBaForMsg;
    int i{0}; //iterator
    int tail, tailNext, head, ready, readyNext;
    std::function<void(RawMsg&&)> cbProcess{nullptr};

    int collectMsg(){
        collectedBaForMsg.clear();
        //case 1:-------TxxxxxxxxRxxxxxH-----
        int startOfMsg{-1};
        const char* packetKeyData = reinterpret_cast<const char*>(&m_ring.buffer[ready].packet.h);
        /*
                // if we use multiple producers, it might be a problem when frames from one msg are mixed.
                // here should be implemented some sorting mechanism that requires to skip packets, for example: TODO TESTS AND BENCHMARKS
                // 1. finding start
                for (int i = ready; i >= tail; i--){
                    auto&& curPacket = m_ring.buffer[i].packet;
                    if (std::memcmp(packetKeyData, reinterpret_cast<const char*>(&curPacket.h), COMPARE_HEADER_KEY_SIZE)){
                        if (curPacket.h.n == 0){
                            startOfMsg = i;
                            break;
                        }
                    }
                }
                // 1a. return if there is no frames.
                if (startOfMsg < 0) return startOfMsg;
                std::vector<std::reference_wrapper<EPacketCell>> vec(m_ring.buffer.begin() + startOfMsg, m_ring.buffer.begin()+ready);  //maybe it's worth to fill the vector inside previous loop?
                // 2 remove all from other packets.
                auto newEnd = std::remove_if(vec.begin(), vec.end(), [&packetKeyData](auto packet){
                    return !std::memcmp(packetKeyData, reinterpret_cast<const char*>(&packet.h), COMPARE_HEADER_KEY_SIZE);
                });
                // 3. sort remains by little n inside header
                std::sort(vec.begin(), newEnd, [](const std::reference_wrapper<EPacketCell>& l, const std::reference_wrapper<EPacketCell>& r){
                    return l.get().packet.h.n < r.get().packet.h.n;
                });
                // 4. fill qbytearray
                for (auto it = vec.begin(); it < newEnd; it++){
                    collectedBaForMsg.append(it->get().packet.data);
                }
                */
        //or wihtout any sort... may be wrong msgs, still need tests, but maybe should use the previous one
        for (int i = ready; i >= tail; i--){
            auto&& curPacket = m_ring.buffer[i].packet;
            if (std::memcmp(packetKeyData, reinterpret_cast<const char*>(&curPacket.h), COMPARE_HEADER_KEY_SIZE)){
                collectedBaForMsg.prepend(curPacket.data);  // THIS CAN BE DIRECTLY MMAPED. IMPLEMENT MMAP TO MEMORY TO AVOID using cbProcess. (Another class or constexpr/template bool for such cases).
                if (curPacket.h.n == 0){
                    startOfMsg = i;
                    cbProcess(RawMsg{RawMsg::Header(curPacket.h), collectedBaForMsg});  // Inside processor -> signal with qobject context, or other mmap with move operation.
                    break;
                }
            }
        }
        return startOfMsg;
    }
    int collectMsgWithOverlap(){
        //case 2: xxRxxxH------Txxxxxx         //watch the previous problem with sorting........ need to sort frames here too...
        int startOfMsg{-1};
        const char* packetKeyData = reinterpret_cast<const char*>(&m_ring.buffer[ready].packet.h);
        bool msgReady{false};
        if (ready > 0){
            for (int i = ready; i >= 0; i--){
                auto&& curPacket = m_ring.buffer[i].packet;
                if (std::memcmp(packetKeyData, reinterpret_cast<const char*>(&curPacket.h), COMPARE_HEADER_KEY_SIZE)){
                    collectedBaForMsg.prepend(curPacket.data);  // THIS CAN BE DIRECTLY MMAPED. IMPLEMENT MMAP TO MEMORY TO AVOID using processor. (Another class or constexpr/template bool for such cases).
                    if (curPacket.h.n == 0){
                        startOfMsg = i;
                        cbProcess(RawMsg{RawMsg::Header(curPacket.h), collectedBaForMsg});  // Inside processor -> signal with qobject context, or other mmap with move operation.
                        msgReady = true;
                        break;
                    }
                }
            }
            if (!msgReady){
                for (int i = FRAME_BUFFER_SIZE -1 ; i >= tail; i++){
                    auto&& curPacket = m_ring.buffer[i].packet;
                    if (std::memcmp(packetKeyData, reinterpret_cast<const char*>(&curPacket.h), COMPARE_HEADER_KEY_SIZE)){
                        collectedBaForMsg.prepend(curPacket.data);  // THIS CAN BE DIRECTLY MMAPED. IMPLEMENT MMAP TO MEMORY TO AVOID using processor. (Another class or constexpr/template bool for such cases).
                        if (curPacket.h.n == 0){
                            startOfMsg = i;
                            cbProcess(RawMsg{RawMsg::Header(curPacket.h), collectedBaForMsg});  // Inside processor -> signal with qobject context, or other mmap with move operation.
                            msgReady = true;
                            break;
                        }
                    }
                }
            }
        }
        else{   // case 2: xxxxH------TxxxxxRxx will be the same for collectMsg
            return collectMsg();
        }
        return startOfMsg;
    }
    bool checkMsgReady(const Packet& packet){
        return (packet.h.n == packet.h.N);
    }


};


#endif // FRAMERINGBUFFER_H
