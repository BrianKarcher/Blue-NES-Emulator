#pragma once
#include <cstdint>
#include <vector>
#include "MemoryMapper.h"

class Cartridge;
class Bus;

class Mapper : public MemoryMapper {
public:
	std::vector<uint8_t> m_prgRomData;
	std::vector<uint8_t> m_prgRamData;
	std::vector<uint8_t> m_chrData;
	bool isCHRWritable;

	virtual void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) = 0;
	virtual inline uint8_t readPRGROM(uint16_t addr) const = 0;
	virtual void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle) = 0;
	virtual inline uint8_t readCHR(uint16_t addr) const = 0;
	virtual void writeCHR(uint16_t addr, uint8_t data) = 0;
	virtual void shutdown() = 0;

	inline uint8_t read(uint16_t address);
	inline void write(uint16_t address, uint8_t value);
	void register_memory(Bus& bus);
};