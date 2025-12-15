// BlueNES.LoadTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <iomanip>
#include <atomic>
#include "Nes.h"

// Cache line flush helper (platform specific)
inline void flushCacheLine(const void* ptr) {
#if defined(_MSC_VER)
    _mm_clflush(ptr);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin___clear_cache((char*)ptr, (char*)ptr + 64);
#endif
}

// Prevent compiler optimizations
volatile uint8_t sink = 0;

//MMC1 mmc;
//Cartridge cart;
std::vector<uint16_t> testAddresses;
std::mt19937 rng;

int main()
{
    std::cout << "Hello World!\n";
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
