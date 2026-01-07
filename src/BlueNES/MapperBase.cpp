#include "MapperBase.h"

void MapperBase::SetPrgPageSize(uint16_t pageSize) {
	_prgPageSize = pageSize;
}

void MapperBase::SetPrgPage(uint16_t pageIndex, uint8_t* data) {
	uint16_t startAddr = pageIndex * _prgPageSize;
	uint16_t endAddr = startAddr + _prgPageSize;
	SetPrgRange(startAddr, endAddr, data);
}

void MapperBase::SetPrgRange(uint16_t startInclusive, uint16_t endExclusive, uint8_t* data) {
	uint8_t startPage = startInclusive >> 8;
	uint8_t endPage = (endExclusive >> 8);
	for (int i = startPage; i < endPage; i++) {
		_prgPages[i] = data;
	}
}

void MapperBase::SetChrPageSize(uint16_t pageSize) {
	_chrPageSize = pageSize;
}

void MapperBase::SetChrPage(uint16_t pageIndex, uint8_t* data) {
	uint16_t startAddr = pageIndex * _chrPageSize;
	uint16_t endAddr = startAddr + _chrPageSize;
	SetChrRange(startAddr, endAddr, data);
}

void MapperBase::SetChrRange(uint16_t startInclusive, uint16_t endExclusive, uint8_t* data) {
	uint8_t startPage = startInclusive >> 8;
	uint8_t endPage = (endExclusive >> 8);
	for (int i = startPage; i < endPage; i++) {
		_chrPages[i] = data;
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