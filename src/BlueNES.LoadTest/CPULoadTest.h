#pragma once
#include "CPULoadTest.h"

#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <iomanip>
#include <thread>
#include <atomic>
#include <algorithm>

#include "Nes.h"
#include "Cartridge.h"
#include "CPU.h"
#include "SharedContext.h"

class CPULoadTest {
private:
    Nes& nes;
    Processor_6502* cpu;
    Cartridge* cart;
    std::mt19937 rng;
    std::vector<uint8_t> testProgram;

    // Volatile sink to prevent optimization
    volatile uint64_t sink = 0;

public:
    CPULoadTest(Nes& nesInstance);

    void generateTestProgram();

    void loadProgram(uint16_t startAddr = 0x8000);

    // Test 1: Raw instruction execution throughput
    void runInstructionThroughputTest(size_t cycles);

    // Test 2: Frame-based execution (realistic usage)
    void runFrameBasedTest(size_t frames);

    // Test 3: Interrupt handling stress test
    void runInterruptStressTest(size_t interrupts);

    // Test 4: Memory-intensive operations
    void runMemoryIntensiveTest(size_t iterations);

    // Test 5: Branch-heavy code
    void runBranchHeavyTest(size_t iterations);

    // Test 6: Multi-threaded CPU instances (if your design supports it)
    void runMultiInstanceTest(size_t iterations, size_t instanceCount);

    // Test 7: Sustained load test (thermal/stability)
    void runSustainedLoadTest(size_t durationSeconds);

private:
    void printResults(const std::string& testName, size_t operations, long long microseconds);

};