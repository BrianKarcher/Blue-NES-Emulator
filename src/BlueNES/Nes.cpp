#include "Nes.h"
#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"
#include "CPU.h"
#include "APU.h"
#include "Input.h"
#include "SharedContext.h"

#define PPU_CYCLES_PER_CPU_CYCLE 3

Nes::Nes(SharedContext& ctx) : cart(bus, cpu) {
    context_ = &ctx;
	bus_ = new Bus(apu, input, cart);
    cpu_ = new Processor_6502();
	cpu_->connectBus(bus_);
	ppu_ = new PPU(ctx, *bus_);
	ppu_->connectBus(bus_);
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