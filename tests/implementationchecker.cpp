#include "implementationchecker.h"
#include <gtest/gtest.h>
#include <algorithm>


bool ImplementationChecker::checkInAndOut(int chunk)
{
    int counter{0};
    bool res = std::equal(inBuf.begin(), inBuf.begin() + (inBuf.size() - (inBuf.size()%chunk)),
                      outBuf.begin(), outBuf.begin() + (outBuf.size() - (outBuf.size()%chunk))
                          ,[&counter](auto in, auto out){
        counter++;
        return in == out;
    });
    return res;
}
