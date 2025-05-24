#pragma once
#include <cstdint>

class Processor_6502
{
	#define FLAG_CARRY     0x01
	#define FLAG_ZERO      0x02
	#define FLAG_INTERRUPT 0x04
	#define FLAG_DECIMAL   0x08
	#define FLAG_BREAK     0x10
	#define FLAG_UNUSED    0x20
	#define FLAG_OVERFLOW  0x40
	#define FLAG_NEGATIVE  0x80

private:
	void adc(uint8_t operand);
public:
	void Initialize(uint8_t* romData, uint8_t* memory);
	void Run();
	void RunStep();
	uint8_t GetA();
	void SetA(uint8_t a);
	uint8_t GetX();
	uint8_t GetY();
	bool GetFlag(uint8_t flag);
	void SetFlag(bool byte, uint8_t flag);
};