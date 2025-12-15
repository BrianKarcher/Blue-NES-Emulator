#pragma once
#include <string>
#include <vector>
#include <random>

class Nes;
class SharedContext;

class BusLoadTest {
private:
    //MMC1 mmc;
    //Cartridge cart;
    Nes& nes;
    std::vector<uint16_t> testAddresses;
    std::mt19937 rng;

public:
    BusLoadTest(Nes& ness, size_t romSize = 256 * 1024);

    void generateRandomAddresses(size_t count);

    // Test with sequential access (worst case for cache avoidance)
    void runSequentialTest(size_t iterations);

    // Test with random access (better for cache avoidance)
    void runRandomTest(size_t iterations);

    // Strided access to maximize cache misses
    void runStridedTest(size_t iterations, size_t stride = 4096);

    // Multi-threaded load test
    void runMultiThreadedTest(size_t iterations, size_t threadCount);

    void printResults(const std::string& testName, size_t operations, long long microseconds);
};