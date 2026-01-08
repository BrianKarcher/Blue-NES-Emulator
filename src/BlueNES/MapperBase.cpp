#include "MapperBase.h"

void MapperBase::initialize(ines_file_t& data) {
	Mapper::initialize(data);
	RecomputeMappings();
}

void MapperBase::SetPrgPageSize(uint16_t pageSize) {
	_prgPageSize = pageSize;
}

void MapperBase::SetPrgPage(uint16_t pageIndex, uint8_t bank) {
	uint16_t startAddr = pageIndex * _prgPageSize;
	uint16_t endAddr = startAddr + _prgPageSize;
	// Calculate the offset into CHR ROM and mask it against total size
	// to prevent out-of-bounds memory access.
	uint32_t totalChrSize = (uint32_t)m_prgRomData.size();
	uint32_t bankOffset = ((uint32_t)bank * _prgPageSize) % totalChrSize;

	SetPrgRange(startAddr, endAddr, bankOffset);
}

void MapperBase::SetPrgRange(uint16_t startInclusive, uint16_t endExclusive, uint32_t bankOffset) {
	// We shift by 8 because the _chrPages array is indexed by 256-byte chunks
	// (0x2000 PPU range / 256 bytes = 32 entries)
	uint8_t startPage = startInclusive >> 8;
	uint8_t endPage = endExclusive >> 8;
	for (int i = startPage; i < endPage; i++) {
		// Calculate how many bytes into the requested bank we are
		uint32_t relativeOffset = (i - startPage) * 256;

		// Map the pointer to the specific 256-byte chunk in the ROM
		_prgPages[i] = &m_prgRomData[bankOffset + relativeOffset];
	}
}

void MapperBase::SetChrPageSize(uint16_t pageSize) {
	_chrPageSize = pageSize;
}

void MapperBase::SetChrPage(uint16_t pageIndex, uint8_t bank) {
	uint16_t startAddr = pageIndex * _chrPageSize;
	uint16_t endAddr = startAddr + _chrPageSize;
	// Calculate the offset into CHR ROM and mask it against total size
	// to prevent out-of-bounds memory access.
	uint32_t totalChrSize = (uint32_t)m_chrData.size();
	uint32_t bankOffset = ((uint32_t)bank * _chrPageSize) % totalChrSize;

	SetChrRange(startAddr, endAddr, bankOffset);
}

void MapperBase::SetChrRange(uint16_t startInclusive, uint16_t endExclusive, uint32_t bankOffset) {
	// We shift by 8 because the _chrPages array is indexed by 256-byte chunks
	// (0x2000 PPU range / 256 bytes = 32 entries)
	uint8_t startPage = startInclusive >> 8;
	uint8_t endPage = endExclusive >> 8;
	for (int i = startPage; i < endPage; i++) {
		// Calculate how many bytes into the requested bank we are
		uint32_t relativeOffset = (i - startPage) * 256;

		// Map the pointer to the specific 256-byte chunk in the ROM
		_chrPages[i] = &m_chrData[bankOffset + relativeOffset];
	}
}

void MapperBase::writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle) {
	writeRegister(address, data, currentCycle);
}

void MapperBase::writeCHR(uint16_t addr, uint8_t data) {
	if (isCHRWritable) {
		_chrPages[addr >> 8][addr & 0xFF] = data;
	}
}

void MapperBase::shutdown() {

}