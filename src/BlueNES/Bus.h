#pragma once
#include <Windows.h>
#include <stdint.h>
#include <array>
#include "cpu.h"
#include "Cartridge.h"

class PPU;
class Core;
class APU;
class Input;

class Bus
{
public:
	Bus();
	~Bus() = default;
	void Initialize(Core* core);

	// Devices connected to the bus
	Processor_6502* cpu = nullptr;
	PPU* ppu = nullptr;
	APU* apu = nullptr;
	Cartridge* cart = nullptr;
	Core* core = nullptr;
	Input* input = nullptr;

	// 2KB internal RAM (mirrored)
	std::array<uint8_t, 2048> cpuRAM{};

	// Access functions
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t data);

	// DMA helper
	void performDMA(uint8_t page);

	void reset();
};