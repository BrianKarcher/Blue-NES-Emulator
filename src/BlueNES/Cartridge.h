#pragma once
#include <cstdint>
#include <string>

enum MirrorMode {
	HORIZONTAL = 0,
	VERTICAL = 1,
	FOUR_SCREEN = 2,
	SINGLE_SCREEN = 3
};

class Cartridge
{
public:
	Cartridge(const std::string& filePath);
	MirrorMode GetMirrorMode();
	uint8_t ReadPRG(uint16_t address);
	uint8_t ReadCHR(uint16_t address);

private:
	MirrorMode mirrorMode;
};