#pragma once
#include <cstdint>
#include "Mapper.h"

class Cartridge;

class NROM : public Mapper {
public:
	NROM(Cartridge* cartridge);
	void writeRegister(uint16_t addr, uint8_t val);
	uint8_t readPRGROM(uint16_t address);
	void writePRGROM(uint16_t address, uint8_t data);
	uint8_t readCHR(uint16_t address);
	void writeCHR(uint16_t address, uint8_t data);
private:
	Cartridge* cartridge;
};