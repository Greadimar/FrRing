#ifndef PCAPCOMMON_H
#define PCAPCOMMON_H
#include <QString>
#include <vector>
#include <pcap/pcap.h>
namespace PcapCommon {
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
struct PcapCatchFilters{
    std::vector<PMac> src;
    std::vector<PMac> dst;
};
struct PcapCapture {
    int snaplen = 1514;    //размер буфера было 65536
    int promisc = 1; // не разборчив в  партнёрах (с) Коля. рекомендуем сетевухе игнорить чужие маки или нет
    int timeout = 0; // каждый таймаут выдаются данные
    int buffer  = 10 * 1024 * 1024; //лучше не трогать
    pcap_direction_t direction = PCAP_D_INOUT;
};
//struct PcapDevice{
//    char* dev;
//    char *net;
//    char *mask;
//    bool opened{false};
//};

class PcapDevice
{
public:
    PcapDevice(const QString &_name, const QString &_description)  : name(_name), description(_description)
    {
        id = ID++;
    }
    PcapDevice(const PcapDevice &pd) :name (pd.getName()), description(pd.getDescription()), id(pd.getId())
    {
    }
    PcapDevice() {}

    QString getName() const {return name;}
    QString getDescription() const { return description; }
    bool open();
    int getId() const { return id; }
    PcapDevice operator=(const PcapDevice &other) noexcept{
        if (this != &other) {
            this->id = other.getId();
            this->description = other.getDescription();
            this->name = other.getName();
        }
        return *this;
    }

private:
    QString name;
    QString description;

    int id;
    static int ID;
};

using errorString = char[PCAP_ERRBUF_SIZE];
#define DEFAULT_SNAPLEN 256

}

#endif // PCAPCOMMON_H
