#pragma once
#include <Windows.h>
#include <stdint.h>
#include <array>
#include "cpu.h"
#include "Cartridge.h"
#include "RAMMapper.h"
#include "MemoryMapper.h"

class PPU;
class APU;
class Input;

class Bus
{
public:
	Bus(Processor_6502& cpu, PPU& ppu, APU& apu, Input& input, Cartridge& cart);
	~Bus();

	RAMMapper ramMapper;
	MemoryMapper** memoryMap; // 64KB memory map

	// Access functions
	void registerAdd(uint16_t start, uint16_t end, MemoryMapper* mapper);
	inline uint8_t read(uint16_t addr);
	inline void write(uint16_t addr, uint8_t data);

	void initialize();

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