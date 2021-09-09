#ifndef CONNECTIONTHREAD_H
#define CONNECTIONTHREAD_H
#include <QThread>
#include <pcap/pcap.h>
#include <QCoreApplication>
static struct timeval startTime;
int pcap_set_application_name(pcap_t *handle, char *name);
class PMac
{
public:
    PMac(const QString &str);
    PMac(const QByteArray &ba);
    ~PMac(){  }
    QByteArray getData() const { return data; }
    QString toString() const;
    bool isValid() const { return valid; }

private:
    QByteArray data;
    bool valid = false;
};
struct Capture {
    int snaplen = 1514;    //размер буфера было 65536
    int promisc = 1; // не разборчив в  партнёрах (с) Коля. рекомендуем сетевухе игнорить чужие маки или нет
    int timeout = 0; // каждый таймаут выдаются данные
    int buffer  = 10 * 1024 * 1024; //лучше не трогать
};
struct PcapDevice{
    char* dev;
    char *net;
    char *mask;
    bool opened{false};
};
using errorString = char[PCAP_ERRBUF_SIZE];
#define DEFAULT_SNAPLEN 256

using PacketHandlerFunc = void(*)(uchar*, const pcap_pkthdr*, const uchar*);
class PcapConnection: public QThread
{
public:
    std::atomic_bool toExit{false};
    bool dbg{false};
    explicit PcapConnection(PcapDevice& device,  const std::vector<PMac> &src, const std::vector<PMac> &dst, const Capture captureSettings, pcap_t* desc):
    pcd(device)
    {
        curHandler = &standartHandler;
    }
    PcapDevice& pcd;
    pcap_t* pd;
    PacketHandlerFunc curHandler;
    static void standartHandler(u_char*, const struct pcap_pkthdr*, const u_char* data){
        //do smth with packet
    }
    void capturePackets() {
      const u_char *pkt;
      struct pcap_pkthdr *h;
      int rc;
      while (!toExit.load(std::memory_order_relaxed)) {
        rc = pcap_next_ex(desc, &h, &pkt);
        if (rc > 0) {
          standartHandler(NULL, h, pkt);
        } else if (rc == 0) {
          /* No packets */
        } else /* rc < 0 */ {
          /* Error */
          break;
        }
      }
    }
    void run(){

        std::string device = "any";
        char c;
        char *bpfFilter = NULL; //ipv4 filter??
        char errbuf[PCAP_ERRBUF_SIZE];
        int promisc = 1;
        int snaplen = DEFAULT_SNAPLEN;
        struct bpf_program fcode;   //ipv4 filter??
        u_int8_t dont_strip_hw_ts = 0;
        pcap_direction_t direction = PCAP_D_INOUT;

        //pcap_t* pd; //descriptor
        if(!dont_strip_hw_ts) setenv("PCAP_PF_RING_STRIP_HW_TIMESTAMP", "1", 1);

        printf("Capturing from %s\n", device.c_str());


        pd = pcap_open_live(device.c_str(), snaplen, promisc, 1000 /* ms */, errbuf);

        if (pd == NULL) {
            printf("pcap_open_live: %s\n", errbuf);
            return;
        }
        if(pcap_setdirection(pd, direction) != 0)
            printf("pcap_setdirection error: '%s'\n", pcap_geterr(pd));

        if(bpfFilter != NULL) {//todo rm?
            if(pcap_compile(pd, &fcode, bpfFilter, 1, 0xFFFFFF00) < 0) {
                printf("pcap_compile error: '%s'\n", pcap_geterr(pd));
            } else {
                if(pcap_setfilter(pd, &fcode) < 0) {
                    printf("pcap_setfilter error: '%s'\n", pcap_geterr(pd));
                }
            }
        }
        std::string appName = QCoreApplication::instance()->applicationName().toStdString().c_str();
        char* name = strdup(appName.c_str());
        pcap_set_application_name(pd, name);

        capturePackets();


        if (dbg){

        }


        pcap_close(pd);
    }
private:
    void displayStats()
    {

        struct pcap_stat pcapStat;
        struct timeval endTime;
        float deltaSec;
        static u_int64_t lastPkts = 0;
        u_int64_t diff;
        static struct timeval lastTime;
        char buf1[64], buf2[64];

        if(startTime.tv_sec == 0) {
            lastTime.tv_sec = 0;
            gettimeofday(&startTime, NULL);
            return;
        }

        gettimeofday(&endTime, NULL);
        deltaSec = (double)delta_time(&endTime, &startTime)/1000000;

        if(pcap_stats(pd, &pcapStat) >= 0) {
            fprintf(stderr, "=========================\n"
            "Absolute Stats: [%u pkts rcvd][%u pkts dropped]\n"
            "Total Pkts=%u/Dropped=%.1f %%\n",
                    pcapStat.ps_recv, pcapStat.ps_drop, pcapStat.ps_recv-pcapStat.ps_drop,
                    pcapStat.ps_recv == 0 ? 0 : (double)(pcapStat.ps_drop*100)/(double)pcapStat.ps_recv);
            fprintf(stderr, "%llu pkts [%.1f pkt/sec] - %llu bytes [%.2f Mbit/sec]\n",
                    numPkts, (double)numPkts/deltaSec,
                    numBytes, (double)8*numBytes/(double)(deltaSec*1000000));

            if(lastTime.tv_sec > 0) {
                deltaSec = (double)delta_time(&endTime, &lastTime)/1000000;
                diff = numPkts-lastPkts;
                fprintf(stderr, "=========================\n"
              "Actual Stats: %s pkts [%.1f ms][%s pkt/sec]\n",
                        pfring_format_numbers(diff, buf1, sizeof(buf1), 0), deltaSec*1000,
                        pfring_format_numbers(((double)diff/(double)(deltaSec)), buf2, sizeof(buf2), 1));
                lastPkts = numPkts;
            }

            fprintf(stderr, "=========================\n");
        }

        lastTime.tv_sec = endTime.tv_sec, lastTime.tv_usec = endTime.tv_usec;
    }
};


class Pcapper{
    Pcapper(){

    }
    QPointer<PcapConnection> makeConnection(const PcapDevice device,  const std::vector<PMac> &src, const std::vector<PMac> &dst, const Capture captureSettings);


};

#endif // CONNECTIONTHREAD_H
