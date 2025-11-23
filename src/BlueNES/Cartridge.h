#pragma once
#include <cstdint>
#include <string>
#include <stdint.h>
#include <array>
#include <vector>
#include "Mapper.h"
#include "INESLoader.h"

class Processor_6502;

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
	Cartridge();
	std::vector<uint8_t> m_prgRomData;
	std::vector<uint8_t> m_prgRamData;
	std::vector<uint8_t> m_chrData;
	bool isCHRWritable;

	void LoadROM(const std::wstring& filePath);
	MirrorMode GetMirrorMode();
	void SetMirrorMode(MirrorMode mirrorMode);
	// Used for testing
	void SetCHRRom(uint8_t* data, size_t size);
	void SetPRGRom(uint8_t* data, size_t size);
	uint8_t ReadPRG(uint16_t address);
	void WritePRG(uint16_t address, uint8_t data);
	uint8_t ReadCHR(uint16_t address);
	// Map a PPU address ($2000–$2FFF) to actual VRAM offset (0–0x7FF)
	uint16_t MirrorNametable(uint16_t addr);
	void WriteCHR(uint16_t address, uint8_t data);
	uint8_t ReadPRGRAM(uint16_t address);
	void WritePRGRAM(uint16_t address, uint8_t data);
	Processor_6502* cpu;
	void SetPrgRamEnabled(bool enable);
	bool isPrgRamEnabled = true;
	void SetMapper(uint8_t value, ines_file_t& inesFile);
private:
	MirrorMode m_mirrorMode;
	Mapper* mapper;
};