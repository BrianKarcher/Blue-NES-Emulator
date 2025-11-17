#pragma once
#include <cstdint>

#define CHR_SMALL 4048 // 4k
#define PRG_SMALL 0x4000 // 16k

class Cartridge;

class MMC1
{
public:
	MMC1(Cartridge* cartridge);
	void writeRegister(uint16_t addr, uint8_t val);
private:
	uint8_t nametableMode;
	uint8_t prgROMMode;
	uint8_t chrROMMode;
	uint8_t shiftRegister;
	void processShift(uint16_t addr, uint8_t val);
	uint8_t chr0Addr;
	uint8_t chr1Addr;
	uint8_t prg0Addr;
	uint8_t prg1Addr;
	bool isActive;
	Cartridge* cartridge;
};