#include "CNROM.h"

CNROM::CNROM() : MapperBase()
{
	MapperBase::SetPrgPageSize(0x8000);
	MapperBase::SetChrPageSize(0x2000);
}

void CNROM::RecomputeMappings() {
	MapperBase::SetPrgPage(0, 0);
	MapperBase::SetChrPage(0, _chrBankReg);
}

void CNROM::writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) {
	// Family Trainer: Jogging Race uses an oversized mapper.
	// Some unlicensed carts also use this mapper with more than 4 CHR banks.
	_chrBankReg = val & 0b1111;
	RecomputeMappings();
}

void CNROM::Serialize(Serializer& serializer) {
	Mapper::Serialize(serializer);
	serializer.Write(_chrBankReg);
}

void CNROM::Deserialize(Serializer& serializer) {
	Mapper::Deserialize(serializer);
	serializer.Read(_chrBankReg);
	RecomputeMappings();
}