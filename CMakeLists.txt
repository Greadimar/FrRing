cmake_minimum_required(VERSION 3.5)

project(frring LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES main.cpp
    src/frring_tools.h
    src/frring_2memcp.h
    src/frring_ifbranches.h
    src/frring_buffer.h
    frring.h)

add_executable(frring ${SOURCES})
add_subdirectory(tests)

