#include "Cartridge.h"
#include <string>
#include "INESLoader.h"

Cartridge::Cartridge() {

}

void Cartridge::LoadROM(const std::wstring& filePath) {
    INESLoader ines;
    ines_file_t* inesFile = ines.load_data_from_ines(filePath.c_str());

    m_prgData.clear();
    for (int i = 0; i < inesFile->prg_rom->size; i++) {
        m_prgData.push_back(inesFile->prg_rom->data[i]);
	}
    m_chrData.clear();
    for (int i = 0; i < inesFile->chr_rom->size; i++) {
        m_chrData.push_back(inesFile->chr_rom->data[i]);
	}
	m_mirrorMode = inesFile->header->flags6 & 0x01 ? VERTICAL : HORIZONTAL;
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
    if (m_prgData.size() < size) {
        m_prgData.resize(size);
	}
    // Pad PRG data to at least 32KB
	// We need to make sure the vectors exist (IRQ vectors at $FFFA-$FFFF)
    // Even if they're zeroes.
    if (m_prgData.size() < 0x8000) {
        m_prgData.resize(0x8000);
    }
    memcpy(m_prgData.data(), data, size);
}

uint8_t Cartridge::ReadPRG(uint16_t address) {
    // If 16 KB PRG ROM, mirror it
    if (m_prgData.size() == 0x4000) {
        // For 16KB PRG-ROM, mask to 14 bits (16KB = 0x4000 bytes)
        return m_prgData[address & 0x3FFF];
    }
	if (address >= 0x8000) {
  //      if (m_prgData.size() == 0) {
  //          return 0; // No PRG data loaded
		//}
		return m_prgData[address - 0x8000];
	}
	return 0;
}

void Cartridge::WritePRG(uint16_t address, uint8_t data)
{
    // Typically, PRG ROM is not writable. This is a placeholder for mappers that support it.
    if (address >= 0x8000) {
        m_prgData[address - 0x8000] = data;
	}
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