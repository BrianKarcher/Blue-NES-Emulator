#pragma once
#include <cstdint>
#include <string>
#include <stdint.h>
#include <array>
#include <vector>
#include "MapperBase.h"
#include "INESLoader.h"
#include <filesystem>

#ifdef _DEBUG
#define LOG(...) dbg(__VA_ARGS__)
#else
#define LOG(...) do {} while(0) // completely removed by compiler
#endif

class CPU;
class Bus;
class SharedContext;

class Cartridge
{
public:
	enum MirrorMode {
		HORIZONTAL = 0,
		VERTICAL = 1,
		SINGLE_LOWER = 2,
		SINGLE_UPPER = 3,
		FOUR_SCREEN = 4
	};
	Cartridge(SharedContext& ctx, CPU& c);
	void connectBus(Bus* bus) { m_bus = bus; }

	void LoadROM(const std::string& filePath);
	MirrorMode GetMirrorMode();
	void SetMirrorMode(MirrorMode mirrorMode);
	// Map a PPU address ($2000–$2FFF) to actual VRAM offset (0–0x7FF)
	uint16_t MirrorNametable(uint16_t addr);
	uint8_t ReadPRGRAM(uint16_t address);
	void WritePRGRAM(uint16_t address, uint8_t data);
	void SetPrgRamEnabled(bool enable);
	bool isPrgRamEnabled = true;
	void SetMapper(uint8_t value, ines_file_t& inesFile);
	void unload();
	bool isLoaded();
	MapperBase* mapper;
	SharedContext& ctx;
	std::wstring fileName;
	std::filesystem::path getAndEnsureSavePath();
private:
	Bus* m_bus;
	MirrorMode m_mirrorMode;
	CPU& cpu;
	void loadSRAM();
	void saveSRAM();
	bool isBatteryBacked = false;
	bool m_isLoaded;
	inline void dbg(const wchar_t* fmt, ...);
};