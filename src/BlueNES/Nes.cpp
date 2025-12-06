#include "Nes.h"

#define PPU_CYCLES_PER_CPU_CYCLE 3

Nes::Nes(SharedContext& ctx) : bus(cpu, ppu, apu, input, cart), context(ctx), ppu(ctx) {
	cpu.connectBus(&bus);
    audioBuffer.reserve(4096);
}

void Nes::clock() {
    ppu.Clock();
    // CPU runs at 1/3 the speed of the PPU
    static int cpuCycleDebt = 0;
    cpuCycleDebt++;

    while (cpuCycleDebt >= PPU_CYCLES_PER_CPU_CYCLE) {
        //cpuCycleDebt -= ppuCyclesPerCPUCycle;
        uint64_t cyclesElapsed = cpu.Clock();
        cpuCycleDebt -= PPU_CYCLES_PER_CPU_CYCLE * cyclesElapsed;

        // Clock APU for each CPU cycle
        for (uint64_t i = 0; i < cyclesElapsed; ++i) {
            apu.step();

            // Generate audio sample based on cycle timing
            audioFraction += 1.0;
            while (audioFraction >= CYCLES_PER_SAMPLE) {
                audioBuffer.push_back(apu.get_output());
                audioFraction -= CYCLES_PER_SAMPLE;
            }
        }
    }
}

bool Nes::frameReady() {
	return ppu.isFrameComplete();
}