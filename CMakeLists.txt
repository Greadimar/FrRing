cmake_minimum_required(VERSION 3.5)

project(frring LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES src/main.cpp
    src/implementations/frring_tools.h
    src/implementations/frring_2memcp.h
    src/implementations/frring_ifbranches.h
    src/implementations/frring_buffer.h
    src/frring.h)
add_subdirectory (tests)
add_executable(frring ${SOURCES})

enable_testing()
add_test (NAME MyTest COMMAND frring_tests)

