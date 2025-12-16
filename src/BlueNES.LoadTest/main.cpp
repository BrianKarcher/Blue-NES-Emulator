#include <iostream>
#include "nes.h"
#include "SharedContext.h"
#include "BlueNES.LoadTest.h"

int main()
{
    return bus();
}

int bus()
{
    std::cout << "MMC1 readPRGROM Load Test" << std::endl;
    std::cout << "=========================" << std::endl;

    // Allocate SharedContext and Nes on the heap
    auto ctx = std::make_unique<SharedContext>();
    auto nes = std::make_unique<Nes>(*ctx);

    BusLoadTest test(*nes, 256 * 1024); // 256KB ROM

    // Run various test patterns
    test.runSequentialTest(100);
    test.runRandomTest(10);
    test.runStridedTest(1000, 4096);
    test.runStridedTest(1000, 8192);
    test.runMultiThreadedTest(100000, 4);

    std::cout << "\n=== Load Test Complete ===" << std::endl;

    return 0;
}

int cpu() {
    std::cout << "NES CPU Emulation Load Test" << std::endl;
    std::cout << "============================" << std::endl;

    auto ctx = std::make_unique<SharedContext>();
    auto nes = std::make_unique<Nes>(*ctx);
    auto test = std::make_unique<CPULoadTest>(*nes);

    // Run comprehensive test suite
    test->runInstructionThroughputTest(1000000);   // 1M cycles
    test->runFrameBasedTest(1000);                  // 1000 frames (~16.6 seconds)
    test->runInterruptStressTest(10000);            // 10K interrupts
    test->runMemoryIntensiveTest(100000);           // 100K memory ops
    test->runBranchHeavyTest(100000);               // 100K branch instructions
    test->runSustainedLoadTest(5);                  // 5 second sustained load

    std::cout << "\n=== CPU Load Test Complete ===" << std::endl;

    return 0;
}