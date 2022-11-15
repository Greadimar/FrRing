#include "implementationchecker.h"
#include <gtest/gtest.h>
#include <algorithm>


bool ImplementationChecker::checkInAndOut()
{
    return std::equal(inBuf.begin(), inBuf.end(), outBuf.begin(), outBuf.end());
}
