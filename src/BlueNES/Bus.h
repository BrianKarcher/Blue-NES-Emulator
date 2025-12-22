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
class OpenBusMapper;
class Serializer;

class Bus
{
public:
	Bus(Processor_6502& cpu, PPU& ppu, APU& apu, Input& input, Cartridge& cart, OpenBusMapper& openBus);
	~Bus();

	RAMMapper ramMapper;
	MemoryMapper** memoryMap; // 64KB memory map

	// Access functions
	void registerAdd(uint16_t start, uint16_t end, MemoryMapper* mapper);
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t data);

	void initialize();

	void Serialize(Serializer& serializer);
	void Deserialize(Serializer& serializer);

	// DMA helper
	void performDMA(uint8_t page);

	void reset();
	bool IrqPending();

	// Devices connected to the bus
	Processor_6502& cpu;
	PPU& ppu;
	APU& apu;
	Cartridge& cart;
	Input& input;
	OpenBusMapper& openBus;

private:

};