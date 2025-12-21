#pragma once
#include <cstdint>
#include "Mapper.h"

class Cartridge;

class NROM : public Mapper {
public:
	NROM(Cartridge* cartridge);
	void writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle);
	inline uint8_t readPRGROM(uint16_t addr) const;
	void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	inline uint8_t readCHR(uint16_t addr) const;
	void writeCHR(uint16_t addr, uint8_t data);
	void shutdown() { }
	void Serialize(Serializer& serializer) override {
		Mapper::Serialize(serializer);
	}
	void Deserialize(Serializer& serializer) override {
		Mapper::Deserialize(serializer);
	}

private:
	Cartridge* cartridge;
};