#pragma once
#include <cstdint>
#include "Mapper.h"
#include "INESLoader.h"

#define CHR_BANK_SIZE 0x1000 // 4k
#define PRG_BANK_SIZE 0x4000 // 16k

enum BoardType {
	SAROM,
	SNROM,
	GenericMMC1
};

class Cartridge;
class Processor_6502;

class MMC1 : public Mapper
{
public:
	MMC1(Cartridge* cartridge, Processor_6502* cpu, uint8_t prgRomSize, uint8_t chrRomSize);
	void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle);
	inline uint8_t readPRGROM(uint16_t addr) const;
	void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	inline uint8_t readCHR(uint16_t addr) const;
	void writeCHR(uint16_t addr, uint8_t data);

	void dbg(const wchar_t* fmt, ...) const;
	BoardType boardType;
private:
	//uint8_t nametableMode;
	//uint8_t prgROMMode;
	//uint8_t chrROMMode;
	uint8_t controlReg = 0x0C;   // default after reset usually 0x0C (mirroring = 0x0, PRG mode = 3)
	uint8_t shiftRegister;
	void processShift(uint16_t addr, uint8_t val);
	void recomputeMappings();
	//uint32_t chr0Addr;
	//uint32_t chr1Addr;
	//uint32_t prg0Addr;
	//uint32_t prg1Addr;
	uint8_t prgBankCount;
	uint8_t chrBankCount;
	uint8_t chrBank0Reg = 0;
	uint8_t chrBank1Reg = 0;
	uint8_t prgBankReg = 0;
	// Current mapped addresses (in bytes)
	uint32_t prg0Addr = 0; // maps to CPU 0x8000-0xBFFF
	uint32_t prg1Addr = 0; // maps to CPU 0xC000-0xFFFF
	uint32_t chr0Addr = 0; // maps to PPU 0x0000-0x0FFF
	uint32_t chr1Addr = 0; // maps to PPU 0x1000-0x1FFF
	Cartridge* cartridge;
	Processor_6502* cpu;
	//ines_file_t* inesFile;
	uint64_t lastWriteCycle = 0;
	bool debug = false;
};