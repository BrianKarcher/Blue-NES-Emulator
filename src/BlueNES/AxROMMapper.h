#pragma once

#include <cstdint>
#include "Mapper.h"

class Cartridge;

class AxROMMapper : public Mapper
{
public:
	AxROMMapper(Cartridge* cartridge, uint8_t prgRom16kSize);
	void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle);
	void recomputeMappings();
	void initialize(ines_file_t& data);
	inline uint8_t readPRGROM(uint16_t addr) const;
	void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	inline uint8_t readCHR(uint16_t addr) const;
	void writeCHR(uint16_t address, uint8_t data);
	void shutdown() {}
	void Serialize(Serializer& serializer) override;
	void Deserialize(Serializer& serializer) override;

private:
	uint8_t* prgAddr = 0;
	uint8_t prgBank32kCount = 0;
	Cartridge* cartridge;
	bool nameTable;
	uint8_t prgBankSelect = 0;
};