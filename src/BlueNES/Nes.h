#pragma once
#include <cstdint>
#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"
#include "CPU.h"
#include "APU.h"
#include "Input.h"

class Nes
{
public:
	Bus bus;
	PPU ppu;
	Cartridge cart;
	Processor_6502 cpu;
	APU apu;
	Input input;

	Nes();

	bool loadRom(const std::wstring& filepath);
	void reset();
	void clock();
	bool frameReady();
};