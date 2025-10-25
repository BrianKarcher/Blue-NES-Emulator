#pragma once
#include <cstdint>
#include <string>
#include <stdint.h>
#include <array>
#include <vector>

enum MirrorMode {
	HORIZONTAL = 0,
	VERTICAL = 1,
	SINGLE_LOWER = 2,
	SINGLE_UPPER = 3,
	FOUR_SCREEN = 4
};

class Cartridge
{
public:
	Cartridge();
	Cartridge(const std::string& filePath);
	MirrorMode GetMirrorMode();
	// Used for testing
	void SetCHRRom(uint8_t* data, size_t size);
	uint8_t ReadPRG(uint16_t address);
	uint8_t ReadCHR(uint16_t address);
	// Map a PPU address ($2000–$2FFF) to actual VRAM offset (0–0x7FF)
	uint16_t MirrorAddress(uint16_t addr);
	void WriteCHR(uint16_t address, uint8_t data);

private:
	std::vector<uint8_t> m_prgData;
	std::vector<uint8_t> m_chrData;
	MirrorMode m_mirrorMode;
};