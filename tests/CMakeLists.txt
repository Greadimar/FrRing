cmake_minimum_required(VERSION 3.5)

#project(frring_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp implementationchecker.cpp )
set(HEADERS implementationchecker.h implementationtests.h )
include_directories(../src)
include_directories(/usr/src/googletest)

set (GTEST_ROOT "/usr/src/googletest")
find_package(GTest CONFIG REQUIRED)
#target_link_libraries(frring_tests STATIC GTest)

#set(GTEST_INCLUDE_DIRS "/usr/src/googletest/googletest/include")
include_directories(${GTest_INCLUDE_DIRS})
message("lol")
message("${GTest_INCLUDE_DIRS}")
message("${GTest_FOUND}")
#add_library(frring_tests STATIC "/usr/src/googletest/googletest/lib/libgtest_main.a")
#target_link_libraries(frring_tests PUBLIC gtest_main)
if (${GTest_FOUND})
    message (STATUS "GTest found")
else()
    message (STATUS "GTest not found") #please install libgtest-dev, and sudo cmake . , sudo --build . -- target install in /usr/src/googletest
endif()
#target_link_libraries(frring_tests "${GTest_BOTH_LIBRARIES}")

add_executable(frring_tests ${SOURCES} ${HEADERS})
target_link_libraries(
  frring_tests PUBLIC
  GTest::gtest_main
)

