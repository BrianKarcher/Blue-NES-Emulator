#include "Cartridge.h"
#include <string>

Cartridge::Cartridge() {

}

Cartridge::Cartridge(const std::string& filePath) {
	// Load the cartridge file and initialize PRG/CHR ROM, mirroring mode, etc.
	m_mirrorMode = MirrorMode::VERTICAL; // Example default
}

// Map a PPU address ($2000–$2FFF) to actual VRAM offset (0–0x7FF)
// Mirror nametable addresses based on mirroring mode
uint16_t Cartridge::MirrorNametable(uint16_t addr) {
    // Get nametable index (0-3) and offset within nametable
    addr &= 0x2FFF;  // Mask to nametable range
    uint16_t offset = addr & 0x03FF;  // Offset within 1KB nametable
    uint16_t table = (addr >> 10) & 0x03;  // Which nametable (0-3)

    // Map logical nametable to physical VRAM based on mirror mode
    switch (m_mirrorMode) {
    case HORIZONTAL:
        // $2000=$2400, $2800=$2C00
        // Tables 0,1 map to first 1KB, tables 2,3 map to second 1KB
        table = (table & 0x02) >> 1;
        break;

    case VERTICAL:
        // $2000=$2800, $2400=$2C00
        // Tables 0,2 map to first 1KB, tables 1,3 map to second 1KB
        table = table & 0x01;
        break;

    case SINGLE_LOWER:
        // All nametables map to first 1KB
        table = 0;
        break;

    case SINGLE_UPPER:
        // All nametables map to second 1KB
        table = 1;
        break;

    case FOUR_SCREEN:
        // No mirroring (requires 4KB of VRAM on cartridge)
        // This would need external RAM on the cartridge
        break;
    }

    return (table * 0x400) + offset;
}

Cartridge::MirrorMode Cartridge::GetMirrorMode() {
	return m_mirrorMode;
}

void Cartridge::SetMirrorMode(MirrorMode mirrorMode) {
    m_mirrorMode = mirrorMode;
}

void Cartridge::SetCHRRom(uint8_t* data, size_t size) {
	m_chrData.resize(size);
	memcpy(m_chrData.data(), data, size);
}

void Cartridge::SetPRGRom(uint8_t* data, size_t size) {
    m_prgData.resize(size);
    memcpy(m_prgData.data(), data, size);
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