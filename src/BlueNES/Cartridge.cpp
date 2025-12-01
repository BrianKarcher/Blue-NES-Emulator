#include "Cartridge.h"
#include <string>
#include "Bus.h"
#include "INESLoader.h"
#include "NROM.h"
#include "Mapper.h"
#include "MMC1.h"
#include "MMC3.h"
#include "CPU.h"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include "UxROMMapper.h"

Cartridge::Cartridge() {

}

void Cartridge::initialize(Bus* bus) {
    cpu = bus->cpu;
    m_bus = bus;
}

std::filesystem::path getAppFolderPath()
{
    wchar_t buf[MAX_PATH];
	// This is a Windows-specific way to get the executable path
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
}

std::filesystem::path Cartridge::getAndEnsureSavePath() {
    std::filesystem::path appFolder = getAppFolderPath();
    std::filesystem::path sramPath = appFolder / "Save";
    std::filesystem::create_directories(sramPath);
    return sramPath;
}

void Cartridge::loadSRAM() {
    m_prgRamData.resize(0x2000);
    if (!isBatteryBacked) {
        return; // No battery-backed SRAM to load
    }
    std::filesystem::path appFolder = getAndEnsureSavePath();
    std::filesystem::path sramFilePath = appFolder / (fileName + L".sav");
    std::ifstream in(sramFilePath, std::ios::binary | std::ios::ate);
    if (!in)
		return; // No SRAM file exists, nothing to load

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    if (!in.read(reinterpret_cast<char*>(m_prgRamData.data()), size))
        throw std::runtime_error("Failed to read file");
}

void Cartridge::saveSRAM() {
    if (!isBatteryBacked)
		return; // No battery-backed SRAM to save
    std::filesystem::path appFolder = getAndEnsureSavePath();
	std::filesystem::path sramFilePath = appFolder / (fileName + L".sav");
    std::ofstream out(sramFilePath, std::ios::binary);
    if (!out)
        throw std::runtime_error("Failed to open output file");

    out.write(reinterpret_cast<const char*>(m_prgRamData.data()), m_prgRamData.size());
}

void Cartridge::unload() {
    saveSRAM();
    if (mapper) {
        delete mapper;
        mapper = nullptr;
    }
    m_prgRomData.clear();
    m_chrData.clear();
	m_prgRamData.clear();
}

void Cartridge::LoadROM(const std::wstring& filePath) {
    INESLoader ines;
    std::filesystem::path filepath(filePath);
    ines_file_t inesFile;
	fileName = filepath.stem().wstring();
    ines.load_data_from_ines(filePath.c_str(), inesFile);
    isBatteryBacked = inesFile.header.flags6 & FLAG_6_BATTERY_BACKED;

    m_prgRomData.clear();
    for (int i = 0; i < inesFile.prg_rom->size; i++) {
        m_prgRomData.push_back(inesFile.prg_rom->data[i]);
	}
    m_chrData.clear();
    if (inesFile.chr_rom->size == 0) {
		isCHRWritable = true;
        // No CHR ROM present; allocate 8KB of CHR RAM
        m_chrData.resize(0x2000, 0);
	}
    else {
        isCHRWritable = false;
        for (int i = 0; i < inesFile.chr_rom->size; i++) {
            m_chrData.push_back(inesFile.chr_rom->data[i]);
        }
    }
	m_mirrorMode = inesFile.header.flags6 & 0x01 ? VERTICAL : HORIZONTAL;
    uint8_t mapperNum = inesFile.header.flags6 >> 4;
    loadSRAM();
    
    SetMapper(mapperNum, inesFile);
}

void Cartridge::SetMapper(uint8_t value, ines_file_t& inesFile) {
    if (mapper) {
        delete mapper;
        mapper = nullptr;
    }

    switch (value) {
    case 0:
        mapper = new NROM(this);
        break;
    case 1:
        mapper = new MMC1(this, cpu, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    case 2:
		mapper = new UxROMMapper(m_bus, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    case 4:
        mapper = new MMC3(m_bus, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    default:
        mapper = new NROM(this);
        break;
    }
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
        // $2000=$2400 (NT1), $2800=$2C00 (NT2)
        // Tables 0,1 map to first 1KB, tables 2,3 map to second 1KB
        table = (table & 0x02) >> 1;
        break;

    case VERTICAL:
        // $2000=$2800 (NT1), $2400=$2C00 (NT2)
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
    if (m_prgRomData.size() < size) {
        m_prgRomData.resize(size);
	}
    // Pad PRG data to at least 32KB
	// We need to make sure the vectors exist (IRQ vectors at $FFFA-$FFFF)
    // Even if they're zeroes.
    if (m_prgRomData.size() < 0x8000) {
        m_prgRomData.resize(0x8000);
    }
    memcpy(m_prgRomData.data(), data, size);
}

uint8_t Cartridge::ReadPRGRAM(uint16_t address) {
    return m_prgRamData[address - 0x6000];
}

void Cartridge::WritePRGRAM(uint16_t address, uint8_t data) {
    m_prgRamData[address - 0x6000] = data;
}

// ---------------- Debug helper ----------------
void dbg2(const wchar_t* fmt, ...) {
    //if (!debug) return;
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
    va_end(args);
    //OutputDebugStringW(buf);
}

void Cartridge::SetPrgRamEnabled(bool enable) {
    dbg2(L"Setting PrgRamEnabled to %d\n", enable);
    isPrgRamEnabled = enable;
}

uint8_t Cartridge::ReadPRG(uint16_t address) {
    return mapper->readPRGROM(address);
}

void Cartridge::WritePRG(uint16_t address, uint8_t data) {
    mapper->writePRGROM(address, data, cpu->GetCycleCount());
}

uint8_t Cartridge::ReadCHR(uint16_t address) {
    return mapper->readCHR(address);
}

// TODO: Support CHR-RAM vs CHR-ROM distinction
void Cartridge::WriteCHR(uint16_t address, uint8_t data) {
    mapper->writeCHR(address, data);
}