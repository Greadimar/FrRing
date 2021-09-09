#ifndef PCAPPRODUCER_H
#define PCAPPRODUCER_H
#include <QThread>
#include <QDebug>
#include "pcapcommon.h"
#include "frameringbuffer.h"
using namespace PcapCommon;


class PcapProducer: public QThread
{
public:
    using PcapErrorString = char[PCAP_ERRBUF_SIZE];

    explicit PcapProducer(FrameRing& ring, PcapCommon::PcapDevice& device,
                          const PcapCatchFilters &filters, const PcapCapture& capture):
        m_ring(ring), m_device(device), m_filters(filters), m_capture(capture){}

    void run(){

        //setup pcap


        char *bpfFilter = NULL; //ipv4 filter??
        struct bpf_program fcode; // TODO check for cases of bpf

        pcap_t* pd; //descriptor
        PcapErrorString errorBuf;
        u_int8_t dont_strip_hw_ts = 0;
        if(!dont_strip_hw_ts) setenv("PCAP_PF_RING_STRIP_HW_TIMESTAMP", "1", 1);


        pd = pcap_open_live(m_device.getName().toLocal8Bit(),
                            m_capture.snaplen,
                            m_capture.promisc,
                            m_capture.timeout, /* ms */
                            errorBuf);
        if (pd == nullptr){
            qWarning() << QString("pcap_open_live error for device %1: %2").arg(m_device.getName()).arg(errorBuf);
            return;
        }
        if (pcap_setdirection(pd, m_capture.direction) != 0){
            qWarning() << QString("pcap_setdirectione error for device %1: %2").arg(m_device.getName()).arg(pcap_geterr(pd));
        }

        if(bpfFilter != NULL) {//todo rm?
            if(pcap_compile(pd, &fcode, bpfFilter, 1, 0xFFFFFF00) < 0) {
                qWarning() << QString("pcap_compile error for device %1: %2").arg(m_device.getName()).arg(pcap_geterr(pd));
            } else {
                if(pcap_setfilter(pd, &fcode) < 0) {
                    qWarning() << QString("pcap_setfilter error for device %1: %2").arg(m_device.getName()).arg(pcap_geterr(pd));
                }
            }
        }

        // now capturing
        const u_char *pkt;
        struct pcap_pkthdr *h;
        while (!m_ring.isRunning.load(std::memory_order_relaxed)) {
            int rc;
            rc = pcap_next_ex(pd, &h, &pkt);
            bool overflowed{false};
            if (rc > 0) {
                do{
                    head = m_ring.lastHead.load(std::memory_order_relaxed);
                    tail = m_ring.lastTail.load(std::memory_order_relaxed);
                    headNext =  (head == FRAME_BUFFER_SIZE - 1)? 0: head+1;

                    if (headNext == tail){
                        qWarning() << "ringbuffer overflowed; can't put a new frame";
                        overflowed = true;
                        continue; // maybe break with loss of packet
                    }
                }
                while (!m_ring.lastHead.compare_exchange_weak(head, headNext, std::memory_order_release, std::memory_order_relaxed));
                // at this point we acquired a new cell for our frame
                auto& epacket = m_ring.buffer[head];
                epacket = reinterpret_cast<EPacketCell&&>(pkt); // let's use move operator because pkt will be destroyed anyway
                while (m_ring.lastHead != head){    // wait
                     QThread::sleep(0);
                }
                m_ring.lastHead = headNext; // relaxed?
            } else if (rc == 0) {
                /* No packets */
            } else /* rc < 0 */ {
                /* Error */
                break;
            }
        }
    }
    void updateFilters(const PcapCatchFilters& filters){

    }


    const PcapCapture &capture() const;

    const PcapCatchFilters &filters() const;

    const PcapCommon::PcapDevice &device() const;

private:
    FrameRing& m_ring;
    PcapCommon::PcapDevice m_device;
    const PcapCatchFilters m_filters;
    const PcapCapture m_capture;
    int tail, head, headNext;
};


#endif // PCAPPRODUCER_H
