#pragma once
#include <cstdint>
#include "Mapper.h"
#include "INESLoader.h"

#define CHR_SMALL 4048 // 4k
#define PRG_SMALL 0x4000 // 16k

class Cartridge;

class MMC1 : public Mapper
{
public:
	MMC1(Cartridge* cartridge, const ines_file_t& inesFile);
	void writeRegister(uint16_t addr, uint8_t val);
	uint8_t readPRGROM(uint16_t address);
	void writePRGROM(uint16_t address, uint8_t data);
	uint8_t readCHR(uint16_t address);
	void writeCHR(uint16_t address, uint8_t data);
private:
	uint8_t nametableMode;
	uint8_t prgROMMode;
	uint8_t chrROMMode;
	uint8_t shiftRegister;
	void processShift(uint16_t addr, uint8_t val);
	uint32_t chr0Addr;
	uint32_t chr1Addr;
	uint32_t prg0Addr;
	uint32_t prg1Addr;
	Cartridge* cartridge;
	//ines_file_t* inesFile;
};