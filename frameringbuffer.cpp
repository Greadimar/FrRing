#include "frameringbuffer.h"

void Consumer::setProcesser(const std::function<void (RawMsg &&)> &newProcesser)
{
    processer = newProcesser;
}
