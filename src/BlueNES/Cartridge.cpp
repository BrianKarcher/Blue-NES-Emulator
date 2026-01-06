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
#include "AxROMMapper.h"
#include "SharedContext.h"

Cartridge::Cartridge(SharedContext& ctx, CPU& c) : cpu(c), ctx(ctx) {
	m_isLoaded = false;
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
    mapper->m_prgRamData.resize(0x2000);
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

    if (!in.read(reinterpret_cast<char*>(mapper->m_prgRamData.data()), size))
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

    out.write(reinterpret_cast<const char*>(mapper->m_prgRamData.data()), mapper->m_prgRamData.size());
}

void Cartridge::unload() {
    saveSRAM();
    if (mapper) {
        mapper->m_prgRomData.clear();
        mapper->m_chrData.clear();
        mapper->m_prgRamData.clear();
		mapper->shutdown();
        delete mapper;
        mapper = nullptr;
    }
    m_mirrorMode = MirrorMode::VERTICAL;
    m_isLoaded = false;
}

void Cartridge::LoadROM(const std::string& filePath) {
    INESLoader ines;
    std::filesystem::path filepath(filePath);
    ines_file_t inesFile;
	fileName = filepath.stem().wstring();
    ines.load_data_from_ines(filePath.c_str(), inesFile);
    // Set the mirror mode now. The mapper may change it later.
    m_mirrorMode = inesFile.header.flags6 & 0x01 ? VERTICAL : HORIZONTAL;
    isBatteryBacked = inesFile.header.flags6 & FLAG_6_BATTERY_BACKED;

    uint8_t mapperNum = inesFile.header.flags6 >> 4;
    SetMapper(mapperNum, inesFile);
	mapper->initialize(inesFile);
	mapper->register_memory(*m_bus);
    loadSRAM();
    
    m_isLoaded = true;
}

void Cartridge::SetMapper(uint8_t value, ines_file_t& inesFile) {
    if (mapper) {
        mapper->shutdown();
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
		mapper = new UxROMMapper(*m_bus, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    case 4:
        mapper = new MMC3(*m_bus, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    case 7:
        mapper = new AxROMMapper(this, inesFile.header.prg_rom_size);
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

uint8_t Cartridge::ReadPRGRAM(uint16_t address) {
    return mapper->m_prgRamData[address - 0x6000];
}

void Cartridge::WritePRGRAM(uint16_t address, uint8_t data) {
    mapper->m_prgRamData[address - 0x6000] = data;
}

// ---------------- Debug helper ----------------
inline void Cartridge::dbg(const wchar_t* fmt, ...) {
    //if (!debug) return;
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
    va_end(args);
    //OutputDebugStringW(buf);
}

void Cartridge::SetPrgRamEnabled(bool enable) {
    LOG(L"Setting PrgRamEnabled to %d\n", enable);
    isPrgRamEnabled = enable;
}

bool Cartridge::isLoaded() {
	return m_isLoaded;
}