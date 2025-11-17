#pragma once
#include <cstdint>
class MMC1
{
public:
	MMC1();
	void writeRegister(uint16_t addr, uint8_t val);
private:
	uint8_t nametableMode;
	uint8_t prgROMMode;
	uint8_t chrROMMode;
	uint8_t shiftRegister;
	void processShift(uint16_t addr);
};