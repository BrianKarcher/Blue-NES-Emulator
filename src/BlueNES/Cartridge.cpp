#include "Cartridge.h"
#include <string>
#include "Bus.h"
#include "INESLoader.h"
#include "NROM.h"
#include "MapperBase.h"
#include "MMC1.h"
#include "MMC3.h"
#include "CPU.h"
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include "UxROMMapper.h"
#include "AxROMMapper.h"
#include "SharedContext.h"
#include "CNROM.h"
#include "MMC2Mapper.h"
#include "MemoryBuffer.h"
#include "DxROM.h"

#include <iostream>
#include <vector>
#include <string>
#include <zip.h> // The libzip header

std::vector<uint8_t> Cartridge::ReadNesFromZip(const std::string& zipPath) {
    int err = 0;

    // 1. Open the ZIP archive
    zip* archive = zip_open(zipPath.c_str(), 0, &err);
    if (!archive) {
        std::cerr << "Failed to open zip file. Error code: " << err << std::endl;
        return std::vector<uint8_t>();
    }

    zip_int64_t numEntries = zip_get_num_entries(archive, 0);
    if (numEntries == 0) {
        std::cerr << "No contents inside Zip file." << err << std::endl;
        return std::vector<uint8_t>();
    }

    // 2. Locate the file inside the ZIP
    zip_stat_t fileStat;
    zip_stat_init(&fileStat);
    // Load whatever is the first file.
	// TODO : Loop through entries to find .nes file
    const char* name = zip_get_name(archive, 0, 0);
    if (zip_stat(archive, name, 0, &fileStat) != 0) {
        std::cerr << "Could not find " << name << " in archive." << std::endl;
        zip_close(archive);
        return std::vector<uint8_t>();
    }

    // 3. Prepare a buffer for the data
    std::vector<uint8_t> buffer(fileStat.size);

    // 4. Open the file within the ZIP and read it
    zip_file* internalFile = zip_fopen(archive, name, 0);
    if (!internalFile) {
        std::cerr << "Failed to open internal file for reading." << std::endl;
        zip_close(archive);
        return std::vector<uint8_t>();
    }

    zip_fread(internalFile, buffer.data(), fileStat.size);
    zip_fclose(internalFile);
    zip_close(archive);

    std::cout << "Successfully loaded " << buffer.size() << " bytes into the buffer." << std::endl;

    // Optional: Verify NES Header (First 3 bytes should be 'N', 'E', 'S')
    if (buffer.size() > 4 && buffer[0] == 'N' && buffer[1] == 'E' && buffer[2] == 'S') {
        std::cout << "Valid iNES header detected!" << std::endl;
    }

    return buffer;
}

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
    m_isLoaded = false;
}

// Helper to convert UTF-16 7z names to std::string
//std::string Cartridge::GetFileName(const CSzArEx* db, UInt32 index) {
//    size_t len = SzArEx_GetFileNameUtf16(db, index, NULL);
//    std::vector<UInt16> temp(len);
//    SzArEx_GetFileNameUtf16(db, index, temp.data());
//    std::string res;
//    for (auto c : temp) if (c) res += (char)c;
//    return res;
//}
//
//std::vector<uint8_t> Cartridge::ReadNesFromZip(const std::string& zipPath) {
//    ISzAlloc allocImp = { SzAlloc, SzFree };
//    ISzAlloc allocTempImp = { SzAlloc, SzFree };
//
//    CFileInStream archiveStream;
//    CLookToRead2 lookStream;
//    CSzArEx db;
//
//    CrcGenerateTable();
//    SzArEx_Init(&db);
//
//    if (InFile_Open(&archiveStream.file, zipPath.c_str()) != 0) return {};
//
//    FileInStream_CreateVTable(&archiveStream);
//    LookToRead2_CreateVTable(&lookStream, False);
//
//    lookStream.bufSize = 1 << 18; // 256KB buffer
//    lookStream.buf = (Byte*)ISzAlloc_Alloc(&allocImp, lookStream.bufSize);
//    lookStream.realStream = &archiveStream.vt;
//    LookToRead2_INIT(&lookStream);
//
//    if (SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp) != SZ_OK) {
//        ISzAlloc_Free(&allocImp, lookStream.buf);
//        File_Close(&archiveStream.file);
//        return {};
//    }
//
//    std::vector<uint8_t> fileData;
//    UInt32 blockIndex = 0xFFFFFFFF;
//    Byte* outBuffer = NULL;
//    size_t outBufferSize = 0;
//
//    for (UInt32 i = 0; i < db.NumFiles; i++) {
//        if (SzArEx_IsDir(&db, i)) continue;
//
//        std::string fileName = GetFileName(&db, i);
//        if (fileName.find(".nes") != std::string::npos) {
//            size_t offset = 0;
//            size_t outSizeProcessed = 0;
//
//            // SzArEx_Extract allocates/reallocates *outBuffer automatically
//            SRes res = SzArEx_Extract(&db, &lookStream.vt, i,
//                &blockIndex, &outBuffer, &outBufferSize,
//                &offset, &outSizeProcessed,
//                &allocImp, &allocTempImp);
//
//            if (res == SZ_OK) {
//                // Copy the specific file data from the uncompressed block
//                fileData.assign(outBuffer + offset, outBuffer + offset + outSizeProcessed);
//            }
//            break;
//        }
//    }
//
//    // Cleanup
//    ISzAlloc_Free(&allocImp, outBuffer);
//    ISzAlloc_Free(&allocImp, lookStream.buf);
//    SzArEx_Free(&db, &allocImp);
//    File_Close(&archiveStream.file);
//
//    return fileData;
//}

std::vector<uint8_t> Cartridge::LoadFileToBuffer(const std::string& path) {
    // Check if it's an archive by extension
    if (path.find(".7z") != std::string::npos || path.find(".zip") != std::string::npos) {
        return ReadNesFromZip(path); // This is the function we built earlier
    }

    // Standard file loading
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) return {};

    size_t size = file.tellg();
    std::vector<uint8_t> buffer(size);
    file.seekg(0, std::ios::beg);
    file.read((char*)buffer.data(), size);
    return buffer;
}

void Cartridge::LoadROM(const std::string& filePath) {
    INESLoader ines;
    std::filesystem::path filepath(filePath);
    ines_file_t inesFile;
	fileName = filepath.stem().wstring();
    std::vector<uint8_t> rawData = LoadFileToBuffer(filePath);
    if (rawData.empty()) {
        MessageBoxA(NULL, "File not found or empty.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    // 2. Wrap it in our memory helper
    MemoryBuffer stream(rawData);
    ines.load_data_from_ines(stream, inesFile);

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
    case 3:
		mapper = new CNROM();
        break;
    case 4:
        mapper = new MMC3(*m_bus, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    case 7:
        mapper = new AxROMMapper(this, inesFile.header.prg_rom_size);
        break;
    case 9:
		mapper = new MMC2Mapper(*m_bus, inesFile.header.prg_rom_size, inesFile.header.chr_rom_size);
        break;
    case 206:
        mapper = new DxROM();
        break;
    default:
        mapper = new NROM(this);
        break;
    }
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