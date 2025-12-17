#include "Nes.h"
#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"
#include "CPU.h"
#include "APU.h"
#include "Input.h"
#include "SharedContext.h"
#include <vector>

#define PPU_CYCLES_PER_CPU_CYCLE 3

Nes::Nes(SharedContext& ctx) {
    context_ = &ctx;
	apu_ = new APU();
    input_ = new Input();
    cpu_ = new Processor_6502();
    cart_ = new Cartridge(*cpu_);
    ppu_ = new PPU(ctx);
    bus_ = new Bus(*cpu_, *ppu_, *apu_, *input_, *cart_);
    bus_->initialize();
	cpu_->connectBus(bus_);
	ppu_->connectBus(bus_);
	cart_->connectBus(bus_);
    ppu_->initialize();
    audioBuffer.reserve(4096);
}

Nes::~Nes() {
    if (bus_) {
        delete bus_;
		bus_ = nullptr;
    }
    if (cpu_) {
        delete cpu_;
		cpu_ = nullptr;
    }
    if (ppu_) {
		delete ppu_;
        ppu_ = nullptr;
    }
    if (cart_) {
        delete cart_;
		cart_ = nullptr;
    }
}

void Nes::clock() {
    ppu_->Clock();
    // CPU runs at 1/3 the speed of the PPU
    static int cpuCycleDebt = 0;
    cpuCycleDebt++;

    while (cpuCycleDebt >= PPU_CYCLES_PER_CPU_CYCLE) {
        //cpuCycleDebt -= ppuCyclesPerCPUCycle;
        uint64_t cyclesElapsed = cpu_->Clock();
        cpuCycleDebt -= PPU_CYCLES_PER_CPU_CYCLE * cyclesElapsed;

        // Clock APU for each CPU cycle
        for (uint64_t i = 0; i < cyclesElapsed; ++i) {
            apu_->step();

            // Generate audio sample based on cycle timing
            audioFraction += 1.0;
            while (audioFraction >= CYCLES_PER_SAMPLE) {
                audioBuffer.push_back(apu_->get_output());
                audioFraction -= CYCLES_PER_SAMPLE;
            }
        }
    }
}

bool Nes::frameReady() {
	return ppu_->isFrameComplete();
}