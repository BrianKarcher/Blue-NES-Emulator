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
#include "NROM.h"

// Cache line flush helper (platform specific)
inline void flushCacheLine(const void* ptr) {
#if defined(_MSC_VER)
    _mm_clflush(ptr);
#elif defined(__GNUC__) || defined(__clang__)
    __builtin___clear_cache((char*)ptr, (char*)ptr + 64);
#endif
}

CPULoadTest::CPULoadTest(Nes& nesInstance)
    : nes(nesInstance)
    , rng(std::random_device{}())
{
    cpu = nesInstance.cpu_;
    cpu->Activate(true);
	cart = nesInstance.cart_;
    cart->mapper = new NROM(nes.cart_);
    generateTestProgram();
}

void CPULoadTest::generateTestProgram() {
    // Generate a realistic NES program with various instruction types
    testProgram = {
        // Common NES instructions mix
        0xA9, 0x00,       // LDA #$00
        0x85, 0x10,       // STA $10
        0xA9, 0xFF,       // LDA #$FF
        0x8D, 0x00, 0x20, // STA $2000
        0xAD, 0x02, 0x20, // LDA $2002
        0x29, 0x80,       // AND #$80
        0xF0, 0xFA,       // BEQ -6
        0xA2, 0x08,       // LDX #$08
        0xA0, 0x00,       // LDY #$00
        0xB9, 0x00, 0x80, // LDA $8000,Y
        0x99, 0x00, 0x02, // STA $0200,Y
        0xC8,             // INY
        0xCA,             // DEX
        0xD0, 0xF5,       // BNE -11
        0x20, 0x00, 0x80, // JSR $8000
        0x60,             // RTS
        0xEA,             // NOP
        0x4C, 0x00, 0x80, // JMP $8000
    };
}

void CPULoadTest::loadProgram(uint16_t startAddr) {
    // Load test program into CPU memory
	cart->m_prgRomData.resize(0x8000); // Ensure enough space
    for (size_t i = 0; i < testProgram.size(); ++i) {
		cart->m_prgRomData[i] = testProgram[i];
    }

    // Set reset vector
    cart->m_prgRomData[0xFFFC - 0x8000] = startAddr & 0xFF;
    cart->m_prgRomData[0xFFFD - 0x8000] = (startAddr >> 8) & 0xFF;
}

void CPULoadTest::runInstructionThroughputTest(size_t cycles) {
    std::cout << "\n=== Instruction Throughput Test ===" << std::endl;

    loadProgram();
	cpu->Reset();

    auto start = std::chrono::high_resolution_clock::now();

    size_t cyclesExecuted = 0;
    while (cyclesExecuted < cycles) {
        flushCacheLine(&nes.cart_->m_prgRomData[0]);
        cyclesExecuted += cpu->Clock();
        sink = cyclesExecuted; // Prevent optimization
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Instruction Throughput", cyclesExecuted, duration.count());

    // Calculate NES clock speed equivalent
    double nesClocksPerSec = (cyclesExecuted * 1000000.0) / duration.count();
    double nesSpeedMultiplier = nesClocksPerSec / 1789773.0; // NTSC NES CPU clock
    std::cout << "  Emulated speed: " << std::fixed << std::setprecision(2)
        << nesSpeedMultiplier << "x NES speed" << std::endl;
}

void CPULoadTest::runFrameBasedTest(size_t frames) {
    std::cout << "\n=== Frame-Based Execution Test ===" << std::endl;

    const size_t CYCLES_PER_FRAME = 29780; // NTSC NES frame

    loadProgram();
    cpu->Reset();

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t frame = 0; frame < frames; ++frame) {
        size_t frameCycles = 0;
        while (frameCycles < CYCLES_PER_FRAME) {
            flushCacheLine(&nes.cart_->m_prgRomData[0]);
            frameCycles += cpu->Clock();
        }
        sink = frameCycles;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    size_t totalCycles = frames * CYCLES_PER_FRAME;
    printResults("Frame-Based", totalCycles, duration.count());

    double framesPerSec = (frames * 1000000.0) / duration.count();
    std::cout << "  Frame rate: " << std::fixed << std::setprecision(2)
        << framesPerSec << " FPS (target: 60 FPS)" << std::endl;
}

void CPULoadTest::runInterruptStressTest(size_t interrupts) {
    std::cout << "\n=== Interrupt Handling Test ===" << std::endl;

    loadProgram();
    cpu->Reset();

    // Setup IRQ vector
	cart->m_prgRomData[0xFFFE - 0x8000] = 0x00; // Low byte of IRQ handler address
	cart->m_prgRomData[0xFFFF - 0x8000] = 0x80; // High byte of IRQ handler address

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < interrupts; ++i) {
        // Execute some instructions
        for (int j = 0; j < 10; ++j) {
            flushCacheLine(&nes.cart_->m_prgRomData[0]);
            cpu->Clock();
        }

        // Trigger IRQ
        cpu->setIRQ(true);

        // Execute interrupt handler
        for (int j = 0; j < 5; ++j) {
            flushCacheLine(&nes.cart_->m_prgRomData[0]);
            cpu->Clock();
        }

        sink = i;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Interrupt Handling", interrupts, duration.count());
}

void CPULoadTest::runMemoryIntensiveTest(size_t iterations) {
    std::cout << "\n=== Memory-Intensive Test ===" << std::endl;

    // Create a program that does lots of memory operations
    std::vector<uint8_t> memProgram = {
        0xA9, 0x00,       // LDA #$00
        0xAA,             // TAX
        0x95, 0x00,       // STA $00,X
        0xE8,             // INX
        0xE0, 0xFF,       // CPX #$FF
        0xD0, 0xF9,       // BNE -7
        0x4C, 0x00, 0x80, // JMP $8000
    };

	cart->m_prgRomData.resize(0x8000); // Ensure enough space
    for (size_t i = 0; i < memProgram.size(); ++i) {
		cart->m_prgRomData[i] = memProgram[i];
    }

    cart->m_prgRomData[0xFFFC - 0x8000] = 0x00;
    cart->m_prgRomData[0xFFFD - 0x8000] = 0x80;
    cpu->Reset();

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        flushCacheLine(&nes.cart_->m_prgRomData[0]);
		cpu->Clock();
        sink = i;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Memory-Intensive", iterations, duration.count());
}

void CPULoadTest::runBranchHeavyTest(size_t iterations) {
    std::cout << "\n=== Branch-Heavy Test ===" << std::endl;

    std::vector<uint8_t> branchProgram = {
        0xA9, 0x00,       // LDA #$00
        0xC9, 0x00,       // CMP #$00
        0xF0, 0x02,       // BEQ +2
        0xA9, 0x01,       // LDA #$01
        0xC9, 0x01,       // CMP #$01
        0xD0, 0x02,       // BNE +2
        0xA9, 0x02,       // LDA #$02
        0x38,             // SEC
        0x90, 0x02,       // BCC +2
        0xA9, 0x03,       // LDA #$03
        0x18,             // CLC
        0xB0, 0x02,       // BCS +2
        0xA9, 0x04,       // LDA #$04
        0x4C, 0x00, 0x80, // JMP $8000
    };

    for (size_t i = 0; i < branchProgram.size(); ++i) {
        cart->m_prgRomData[i] = branchProgram[i];
    }

    branchProgram[0xFFFC] = 0x00;
    branchProgram[0xFFFD] = 0x80;
    cpu->Reset();

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < iterations; ++i) {
        flushCacheLine(&nes.cart_->m_prgRomData[0]);
		cpu->Clock();
        sink = i;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Branch-Heavy", iterations, duration.count());
}

void CPULoadTest::runMultiInstanceTest(size_t iterations, size_t instanceCount) {
    std::cout << "\n=== Multi-Instance Test (" << instanceCount << " CPUs) ===" << std::endl;

    std::atomic<size_t> completedOps{ 0 };
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (size_t t = 0; t < instanceCount; ++t) {
        threads.emplace_back([&, t]() {
            // Each thread would need its own NES instance
            // This is a simplified version
            for (size_t i = 0; i < iterations; ++i) {
                flushCacheLine(&nes.cart_->m_prgRomData[0]);
                cpu->Clock();
                completedOps.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Multi-Instance", completedOps.load(), duration.count());
}

// Test 7: Sustained load test (thermal/stability)
void CPULoadTest::runSustainedLoadTest(size_t durationSeconds) {
    std::cout << "\n=== Sustained Load Test (" << durationSeconds << "s) ===" << std::endl;

    loadProgram();
    cpu->Reset();

    auto start = std::chrono::high_resolution_clock::now();
    auto endTime = start + std::chrono::seconds(durationSeconds);

    size_t cyclesExecuted = 0;
    while (std::chrono::high_resolution_clock::now() < endTime) {
        flushCacheLine(&nes.cart_->m_prgRomData[0]);
        cpu->Clock();
        cyclesExecuted++;
        sink = cyclesExecuted;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    printResults("Sustained Load", cyclesExecuted, duration.count());
}

void CPULoadTest::printResults(const std::string& testName, size_t operations, long long microseconds) {
    double opsPerSecond = (operations * 1000000.0) / microseconds;
    double nsPerOp = (microseconds * 1000.0) / operations;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << testName << " Results:" << std::endl;
    std::cout << "  Total operations: " << operations << std::endl;
    std::cout << "  Time elapsed: " << microseconds / 1000.0 << " ms" << std::endl;
    std::cout << "  Throughput: " << opsPerSecond / 1000000.0 << " million ops/sec" << std::endl;
    std::cout << "  Latency: " << nsPerOp << " ns/op" << std::endl;
}