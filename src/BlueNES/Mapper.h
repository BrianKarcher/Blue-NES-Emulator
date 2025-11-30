#pragma once
#include <cstdint>

class Mapper {
public:
	virtual void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) = 0;
	virtual inline uint8_t readPRGROM(uint16_t addr) const = 0;
	virtual void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle) = 0;
	virtual inline uint8_t readCHR(uint16_t addr) const = 0;
	virtual void writeCHR(uint16_t addr, uint8_t data) = 0;
};