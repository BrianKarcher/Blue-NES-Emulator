#include "Cartridge.h"
#include <string>

Cartridge::Cartridge() {

}

Cartridge::Cartridge(const std::string& filePath) {
	// Load the cartridge file and initialize PRG/CHR ROM, mirroring mode, etc.
	m_mirrorMode = MirrorMode::VERTICAL; // Example default
}

// Map a PPU address ($2000–$2FFF) to actual VRAM offset (0–0x7FF)
uint16_t Cartridge::MirrorAddress(uint16_t addr)
{
	uint8_t mirrorMode = MirrorMode::VERTICAL; // m_ppuCtrl & 0x03; // Bits 0-1 of PPUCTRL determine nametable
	addr = (addr - 0x2000) & 0x0FFF; // Normalize into 0x000–0xFFF (4KB range)
	uint16_t table = addr / 0x400;   // Which of the 4 logical nametables
	uint16_t offset = addr % 0x400;  // Offset within that table

	switch (mirrorMode)
	{
	case MirrorMode::VERTICAL:
		// NT0 and NT2 -> physical 0
		// NT1 and NT3 -> physical 1
		// pattern: 0,1,0,1
		return (table % 2) * 0x400 + offset;

	case MirrorMode::HORIZONTAL:
		// NT0 and NT1 -> physical 0
		// NT2 and NT3 -> physical 1
		// pattern: 0,0,1,1
		return ((table / 2) * 0x400) + offset;

	case MirrorMode::SINGLE_LOWER:
		// All nametables map to 0x000
		return 0x000 + offset;

	case MirrorMode::SINGLE_UPPER:
		return 0x400 + offset;

	case MirrorMode::FOUR_SCREEN:
		// Cartridge provides 4KB VRAM, so direct mapping
		return addr; // No mirroring

	default:
		return 0; // Safety
	}
}

MirrorMode Cartridge::GetMirrorMode() {
	return m_mirrorMode;
}

void Cartridge::SetCHRRom(uint8_t* data, size_t size) {
	m_chrData.resize(size);
	memcpy(m_chrData.data(), data, size);
}

uint8_t Cartridge::ReadPRG(uint16_t address) {
	if (address >= 0x8000) {
		return m_prgData[address - 0x8000];
	}
	return 0;
}

uint8_t Cartridge::ReadCHR(uint16_t address) {
	if (address < 0x2000) {
		return m_chrData[address];
	}
	return 0;
}

// TODO: Support CHR-RAM vs CHR-ROM distinction
void Cartridge::WriteCHR(uint16_t address, uint8_t data) {
	if (address < 0x2000) {
		m_chrData[address] = data;
	}
}