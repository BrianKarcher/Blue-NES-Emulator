#pragma once
#include <stdint.h>
#include <array>
#include "Mapper.h"

class Bus;
class Cartridge;
class Processor_6502;
class RendererLoopy;

//#define UXROMDEBUG

class UxROMMapper : public Mapper
{
public:
	UxROMMapper(Bus& bus, uint8_t prgRomSize, uint8_t chrRomSize);

	void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle);
	//uint8_t readPRGROM(uint16_t address);
	//void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	inline uint8_t readCHR(uint16_t addr) const;
	void writeCHR(uint16_t addr, uint8_t data);
	inline uint8_t readPRGROM(uint16_t addr) const;
	void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	void ClockIRQCounter(uint16_t ppu_address);
	void shutdown() {}

private:
	inline void dbg(const wchar_t* fmt, ...);
	// Mapper state
	uint8_t prg_bank_select;  // Selected 16KB bank for $8000-$BFFF
	std::array<uint8_t*, 2> prgMap;
	uint8_t prgBank16kCount;

	Cartridge* cart;
	Processor_6502& cpu;
	Bus& bus;

	void recomputeMappings();
};