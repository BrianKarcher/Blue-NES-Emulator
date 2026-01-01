#pragma once
#include <stdint.h>
#include <array>
#include "Mapper.h"

class Bus;
class Cartridge;
class CPU;
class RendererLoopy;

#ifdef _DEBUG
#define LOG(...) dbg(__VA_ARGS__)
#else
#define LOG(...) do {} while(0) // completely removed by compiler
#endif

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
	void initialize(ines_file_t& data);
	void Serialize(Serializer& serializer) override;
	void Deserialize(Serializer& serializer) override;

private:
	inline void dbg(const wchar_t* fmt, ...);
	// Mapper state
	uint8_t prg_bank_select;  // Selected 16KB bank for $8000-$BFFF
	uint8_t prgBank16kCount;

	Cartridge* cart;
	CPU& cpu;
	Bus& bus;

	void recomputeMappings();
};