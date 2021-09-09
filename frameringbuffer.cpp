#include "frameringbuffer.h"


void Consumer::setCbProcess(const std::function<void(RawMsg &&)> &newCbProcess)
{
    cbProcess = newCbProcess;
}
