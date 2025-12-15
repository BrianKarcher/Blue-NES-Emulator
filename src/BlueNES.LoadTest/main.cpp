#include <iostream>
#include "nes.h"
#include "SharedContext.h"
#include "BlueNES.LoadTest.h"

int main()
{
    //std::cout << "MMC1 readPRGROM Load Test" << std::endl;
    //std::cout << "=========================" << std::endl;

    //// Allocate SharedContext and Nes on the heap
    //auto ctx = std::make_unique<SharedContext>();
    //auto nes = std::make_unique<Nes>(*ctx);

    //BusLoadTest test(*nes, 256 * 1024); // 256KB ROM

    //// Run various test patterns
    //test.runSequentialTest(100);
    //test.runRandomTest(10);
    //test.runStridedTest(1000, 4096);
    //test.runStridedTest(1000, 8192);
    //test.runMultiThreadedTest(100000, 4);

    //std::cout << "\n=== Load Test Complete ===" << std::endl;

    return 0;
}