#include <iostream>
#include "implementationtests.h"
#include <gtest/gtest.h>
using namespace std;

int main(int argc, char* argv[])
{
    //ImplementationChecker ch(FrRingSettings{});
    cout << "TESTS" << endl;
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
