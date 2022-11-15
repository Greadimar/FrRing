#ifndef FRRING_TOOLS_H
#define FRRING_TOOLS_H
#include <atomic>
#include <bit>
#include <iostream>
#define CACHE_LINE_SIZE 64 // 64 bytes cache line size for x86-64 processors // tosdfsd
namespace FrRingDetails{

constexpr int CIRCULAR_BUFFER_SIZE = 0x1000000; // power of 2
struct alignas(CACHE_LINE_SIZE) LocalState
{
    char* buffer;
    int pos;
};
struct alignas(CACHE_LINE_SIZE) SharedState
{
    std::atomic_int pos;
};

constexpr bool is_power_of_two(int x)
{
    return x && ((x & (x-1)) == 0);
}

struct alignas(CACHE_LINE_SIZE) StatMonitor{
    std::atomic_int declines{0};        //buffer is full (can't write), or buffer is empty(can't read)
    std::atomic_int overlaps{0};
};

}
#endif // FRRING_TOOLS_H
