// Nes.h

#pragma once
#include <cstdint>

const double CPU_FREQ = 1789773.0;
const double CYCLES_PER_SAMPLE = CPU_FREQ / 44100.0;  // 40.58 exact
const int TARGET_SAMPLES_PER_FRAME = 735; // 44100 / 60 = 735 samples per frame

class Bus;
class PPU;
class Cartridge;
class Processor_6502;
class APU;
class Input;

class Nes
{
public:
	Bus& bus() { return *bus_; }
	PPU& ppu() { return *ppu_; }
	Cartridge& cart() { return *cart_; }
	Processor_6502& cpu() { return *cpu_; }
	APU& apu() { return *apu_; }
	Input& input() { return *input_; }
	SharedContext& context() { return *context_; }

	Nes(SharedContext& ctx);

	bool loadRom(const std::wstring& filepath);
	void reset();
	void clock();
	bool frameReady();
	// Audio buffer for queueing samples
	std::vector<float> audioBuffer;

private:
	Bus* bus_;
	PPU* ppu_;
	Cartridge* cart_;
	Processor_6502* cpu_;
	APU* apu_;
	Input* input_;
	SharedContext* context_;

	double audioFraction = 0.0;  // Per-frame fractional pos
};