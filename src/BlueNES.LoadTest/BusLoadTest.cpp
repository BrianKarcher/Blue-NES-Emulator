#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <iomanip>
#include <atomic>
#include "Nes.h"
#include "BlueNES.LoadTest.h"
#include "Cartridge.h"
#include "Bus.h"
#include "NROM.h"

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

BusLoadTest::BusLoadTest(Nes& ness, size_t romSize) : nes(ness), rng(std::random_device{}()) {
    nes.cart_->m_prgRomData.resize(romSize);
	nes.cart_->m_prgRamData.resize(0x2000);
	nes.cart_->m_chrData.resize(0x2000);
	nes.cart_->mapper = new NROM(nes.cart_);
    // Initialize cartridge with random data
    for (auto& byte : nes.cart_->m_prgRomData) {
        byte = static_cast<uint8_t>(rng() & 0xFF);
    }

    // Setup MMC1
    //mmc.cartridge = &cart;
    //mmc.prg0Addr = 0;
    //mmc.prg1Addr = romSize / 2;

    // Generate randomized test addresses to defeat cache prediction
    generateRandomAddresses(100000);
}

void BusLoadTest::generateRandomAddresses(size_t count) {
    testAddresses.clear();
    testAddresses.reserve(count);

    std::uniform_int_distribution<uint16_t> dist(0x0000, 0xFFFF);
    for (size_t i = 0; i < count; ++i) {
        testAddresses.push_back(dist(rng));
    }
}

// Test with sequential access (worst case for cache avoidance)
void BusLoadTest::runSequentialTest(int iterations) {
    std::cout << "\n=== Sequential Access Test ===" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        for (uint32_t addr = 0x8000; addr < 0xFFFF; addr += 64) {
            // Flush the cache line containing the result
            flushCacheLine(&nes.cart_->m_prgRomData[0]);

            //uint8_t result = mmc.readPRGROM(addr);
            uint8_t result = nes.bus_->read(addr);
            sink = result; // Prevent optimization
			//std::cout << "Read from " << std::hex << addr << ": " << std::dec << (int)result << std::endl;
        }
		//std::cout << "Iteration " << i << " complete." << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    size_t totalOps = iterations * ((0xFFFF - 0x8000) / 64);
    printResults("Sequential", totalOps, duration.count());
}

// Test with random access (better for cache avoidance)
void BusLoadTest::runRandomTest(int iterations) {
    std::cout << "\n=== Random Access Test ===" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        for (const auto& addr : testAddresses) {
            // Flush cache periodically
            if ((i & 0x3F) == 0) {
                flushCacheLine(&nes.cart_->m_prgRomData[addr & 0x3FFF]);
            }

            uint8_t result = nes.bus_->read(addr);
            sink = result;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    size_t totalOps = iterations * testAddresses.size();
    printResults("Random", totalOps, duration.count());
}

// Strided access to maximize cache misses
void BusLoadTest::runStridedTest(int iterations, size_t stride) {
    std::cout << "\n=== Strided Access Test (stride=" << stride << ") ===" << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        for (uint32_t addr = 0x8000; addr < 0xFFFF; addr += stride) {
            // Large stride should naturally avoid cache hits
            uint8_t result = nes.bus_->read(addr);
            sink = result;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    size_t totalOps = iterations * ((0xFFFF - 0x8000) / stride);
    printResults("Strided", totalOps, duration.count());
}

// Multi-threaded load test
void BusLoadTest::runMultiThreadedTest(int iterations, size_t threadCount) {
    std::cout << "\n=== Multi-threaded Test (" << threadCount << " threads) ===" << std::endl;

    std::atomic<size_t> completedOps{ 0 };
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t t = 0; t < threadCount; ++t) {
        threads.emplace_back([&, t]() {
            // Each thread gets its own random seed
            std::mt19937 localRng(t);
            std::uniform_int_distribution<uint16_t> dist(0x8000, 0xFFFF);

            for (size_t i = 0; i < iterations; ++i) {
                uint16_t addr = dist(localRng);
                uint8_t result = nes.bus_->read(addr);
                sink = result;
                completedOps.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Multi-threaded", completedOps.load(), duration.count());
}

void BusLoadTest::printResults(const std::string& testName, size_t operations, long long microseconds) {
    double opsPerSecond = (operations * 1000000.0) / microseconds;
    double nsPerOp = (microseconds * 1000.0) / operations;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << testName << " Test Results:" << std::endl;
    std::cout << "  Total operations: " << operations << std::endl;
    std::cout << "  Time elapsed: " << microseconds / 1000.0 << " ms" << std::endl;
    std::cout << "  Throughput: " << opsPerSecond / 1000000.0 << " million ops/sec" << std::endl;
    std::cout << "  Latency: " << nsPerOp << " ns/op" << std::endl;
}