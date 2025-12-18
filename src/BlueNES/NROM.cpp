#include "NROM.h"
#include "Cartridge.h"

NROM::NROM(Cartridge* cartridge) {
	this->cartridge = cartridge;
}

void NROM::writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) {

}

inline uint8_t NROM::readPRGROM(uint16_t address) const {
	if (m_prgRomData.size() == 0) {
		return 0;
	}
	// If 16 KB PRG ROM, mirror it
	if (m_prgRomData.size() == 0x4000) {
		// For 16KB PRG-ROM, mask to 14 bits (16KB = 0x4000 bytes)
		return m_prgRomData[address & 0x3FFF];
	}
	if (address >= 0x8000) {
		//      if (m_prgData.size() == 0) {
		//          return 0; // No PRG data loaded
			  //}
		return m_prgRomData[address - 0x8000];
	}
	return 0;
}

void NROM::writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle)
{
	// Typically, PRG ROM is not writable. This is a placeholder for mappers that support it.
}

inline uint8_t NROM::readCHR(uint16_t address) const {
	if (address < 0x2000) {
		return m_chrData[address];
	}
	return 0;
}

// TODO: Support CHR-RAM vs CHR-ROM distinction
void NROM::writeCHR(uint16_t address, uint8_t data) {
	if (!isCHRWritable) {
		return; // Ignore writes if CHR is ROM
	}
	if (address < 0x2000) {
		m_chrData[address] = data;
	}
}