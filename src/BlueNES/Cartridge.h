#pragma once
#include <cstdint>
#include <string>
#include <stdint.h>
#include <array>
#include <vector>
#include "Mapper.h"
#include "INESLoader.h"
#include <filesystem>

class Processor_6502;
class Bus;

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
	Cartridge(Processor_6502& c);
	void connectBus(Bus* bus) { m_bus = bus; }

	void LoadROM(const std::wstring& filePath);
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
	Mapper* mapper;
private:
	Bus* m_bus;
	MirrorMode m_mirrorMode;
	Processor_6502& cpu;
	std::filesystem::path getAndEnsureSavePath();
	void loadSRAM();
	void saveSRAM();
	std::wstring fileName;
	bool isBatteryBacked = false;
	bool m_isLoaded;
};