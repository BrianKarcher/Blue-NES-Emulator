#pragma once
#include <stdint.h>
#include <array>
#include "Mapper.h"

class Cartridge;
class Processor_6502;

class MMC3 : public Mapper
{
public:
	MMC3(Cartridge* cartridge, Processor_6502* cpu, uint8_t prgRomSize, uint8_t chrRomSize);

	void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle);
	//uint8_t readPRGROM(uint16_t address);
	//void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	inline uint8_t readCHR(uint16_t addr) const;
	void writeCHR(uint16_t addr, uint8_t data);
	inline uint8_t readPRGROM(uint16_t addr) const;
	void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);

private:
	uint8_t prgMode;
	uint8_t chrMode;
	std::array<uint8_t*, 8> chrMap;
	std::array<uint8_t*, 4> prgMap;
	uint8_t banks[8] = { 0 };          // raw bank numbers (after masking)
	uint8_t prgBank16kCount;
	uint8_t prgBank8kCount;
	uint8_t chrBank8kCount;
	uint8_t chrBank1kCount;

	uint8_t m_regSelect;

	Cartridge* cart;
	Processor_6502* cpu;

	void recomputeMappings();
	void updateChrMapping();
	void updatePrgMapping();
};