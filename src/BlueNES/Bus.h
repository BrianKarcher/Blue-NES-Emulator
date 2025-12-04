#pragma once
#include <Windows.h>
#include <stdint.h>
#include <array>
#include "cpu.h"
#include "Cartridge.h"

class PPU;
class APU;
class Input;

class Bus
{
public:
	Bus(Processor_6502& cpu, PPU& ppu, APU& apu, Input& input, Cartridge& cart);
	~Bus() = default;

	// 2KB internal RAM (mirrored)
	std::array<uint8_t, 2048> cpuRAM{};

	// Access functions
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t data);

	// DMA helper
	void performDMA(uint8_t page);

	void reset();

	// Devices connected to the bus
	Processor_6502& cpu;
	PPU& ppu;
	APU& apu;
	Cartridge& cart;
	Input& input;

private:

};