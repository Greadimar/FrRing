#include "pcapproducer.h"



const PcapCapture &PcapProducer::capture() const
{
    return m_capture;
}

const PcapCatchFilters &PcapProducer::filters() const
{
    return m_filters;
}

const PcapCommon::PcapDevice &PcapProducer::device() const
{
    return m_device;
}
