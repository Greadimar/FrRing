cmake_minimum_required(VERSION 3.5)

project(frring_tests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp implementationchecker.cpp)
set(HEADERS implementationchecker.h)
include_directories(../src)

add_executable(frring_tests ${SOURCES} ${HEADERS})

