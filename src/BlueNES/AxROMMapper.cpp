#include "AxROMMapper.h"
#include "Cartridge.h"

AxROMMapper::AxROMMapper(Cartridge* cartridge, uint8_t prgRom16kSize) : cartridge(cartridge) {
	prgBank32kCount = prgRom16kSize / 2;
	//prgBankSelect = prgBank32kCount - 1; // Default to last bank
	prgBankSelect = 0;
}

void AxROMMapper::writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) {
	prgBankSelect = val & 0x07; // Select 32KB PRG bank (3 bits)
	nameTable = val & 0x10;
}

void AxROMMapper::initialize(ines_file_t& data) {
	Mapper::initialize(data);
	recomputeMappings();
}

void AxROMMapper::recomputeMappings() {
	prgAddr = &m_prgRomData[prgBankSelect * 0x8000];
	cartridge->SetMirrorMode(nameTable ? Cartridge::MirrorMode::SINGLE_UPPER : Cartridge::MirrorMode::SINGLE_LOWER);
}

inline uint8_t AxROMMapper::readPRGROM(uint16_t addr) const {
	return prgAddr[addr & 0x7FFF];
}

void AxROMMapper::writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle) {
	writeRegister(address, data, currentCycle);
}

inline uint8_t AxROMMapper::readCHR(uint16_t addr) const {
	// This mapper uses CHR-RAM only.
	return m_chrData[addr];
}

void AxROMMapper::writeCHR(uint16_t address, uint8_t data) {
	if (!isCHRWritable) {
		return; // Ignore writes if CHR is ROM
	}
	m_chrData[address & 0x1FFF] = data;
}