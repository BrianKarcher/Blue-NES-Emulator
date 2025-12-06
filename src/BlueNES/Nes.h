#pragma once
#include <cstdint>
#include "Bus.h"
#include "PPU.h"
#include "Cartridge.h"
#include "CPU.h"
#include "APU.h"
#include "Input.h"
#include "SharedContext.h"

const double CPU_FREQ = 1789773.0;
const double CYCLES_PER_SAMPLE = CPU_FREQ / 44100.0;  // 40.58 exact
const int TARGET_SAMPLES_PER_FRAME = 735; // 44100 / 60 = 735 samples per frame

class Nes
{
public:
	Bus bus;
	PPU ppu;
	Cartridge cart;
	Processor_6502 cpu;
	APU apu;
	Input input;
	SharedContext& context;

	Nes(SharedContext& ctx);

	bool loadRom(const std::wstring& filepath);
	void reset();
	void clock();
	bool frameReady();
	// Audio buffer for queueing samples
	std::vector<float> audioBuffer;

private:
	double audioFraction = 0.0;  // Per-frame fractional pos
};