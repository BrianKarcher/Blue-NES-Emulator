#pragma once
#include "Mapper.h"

// TODO : Integrate common mapper functionality here
class MapperBase : public Mapper {
	// Converting all PRG and CHR mapping to use pages.
	// For simplicity across all mappers. All mappers have window sizes >= 256 bytes.
	// This also increases performance by reducing the number of calculations needed for each read/write.

public:
	uint8_t* _prgPages[0x100];
	uint16_t _prgPageSize = 0;
	uint8_t* _chrPages[0x100];
	uint16_t _chrPageSize = 0;

	virtual void RecomputeMappings() = 0;
	void initialize(ines_file_t& data) override;
	void SetPrgPageSize(uint16_t pageSize);
	void SetPrgPage(uint16_t pageIndex, uint8_t bank);
	void SetPrgRange(uint16_t startInclusive, uint16_t endExclusive, uint32_t bankOffset);

	void SetChrPageSize(uint16_t pageSize);
	void SetChrPage(uint16_t pageIndex, uint8_t bank);
	void SetChrRange(uint16_t startInclusive, uint16_t endExclusive, uint32_t bankOffset);

	inline uint8_t readPRGROM(uint16_t addr) const {
		addr -= 0x8000;
		return _prgPages[addr >> 8][addr & 0xFF];
	}

	void writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle);
	virtual inline uint8_t readCHR(uint16_t addr) {
		return _chrPages[addr >> 8][addr & 0xFF];
	}

	void writeCHR(uint16_t addr, uint8_t data);
	void shutdown();
};