#pragma once
#include <Windows.h>
#include <stdint.h>
#include <array>
#include "processor_6502.h"
#include "nes_ppu.h"

class Bus
{
public:
	Bus();
	~Bus() = default;

	// Devices connected to the bus
	Processor_6502* cpu = nullptr;
	NesPPU* ppu = nullptr;

	// 2KB internal RAM (mirrored)
	std::array<uint8_t, 2048> cpuRAM{};

	// Access functions
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t data);

	// DMA helper
	void performDMA(uint8_t page);

private:
	
};