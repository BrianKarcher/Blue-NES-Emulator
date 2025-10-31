#pragma once
#include <cstdint>

#define FLAG_CARRY     0x01
#define FLAG_ZERO      0x02
#define FLAG_INTERRUPT 0x04
#define FLAG_DECIMAL   0x08
#define FLAG_BREAK     0x10
#define FLAG_UNUSED    0x20
#define FLAG_OVERFLOW  0x40
#define FLAG_NEGATIVE  0x80

// op codes
const uint8_t ADC_IMMEDIATE = 0x69;
const uint8_t ADC_ZEROPAGE = 0x65;
const uint8_t ADC_ZEROPAGE_X = 0x75;
const uint8_t ADC_ABSOLUTE = 0x6D;
const uint8_t ADC_ABSOLUTE_X = 0x7D;
const uint8_t ADC_ABSOLUTE_Y = 0x79;
const uint8_t ADC_INDEXEDINDIRECT = 0x61;
const uint8_t ADC_INDIRECTINDEXED = 0x71;
const uint8_t AND_IMMEDIATE = 0x29;
const uint8_t AND_ZEROPAGE = 0x25;
const uint8_t AND_ZEROPAGE_X = 0x35;
const uint8_t AND_ABSOLUTE = 0x2D;
const uint8_t AND_ABSOLUTE_X = 0x3D;
const uint8_t AND_ABSOLUTE_Y = 0x39;
const uint8_t AND_INDEXEDINDIRECT = 0x21;
const uint8_t AND_INDIRECTINDEXED = 0x31;
const uint8_t ASL_ACCUMULATOR = 0x0A;
const uint8_t ASL_ZEROPAGE = 0x06;
const uint8_t ASL_ZEROPAGE_X = 0x16;

class Bus;

class Processor_6502
{
public:
	void Initialize();
	void Clock();
	uint8_t GetA();
	void SetA(uint8_t a);
	uint8_t GetX();
	void SetX(uint8_t x);
	uint8_t GetY();
	void SetY(uint8_t y);
	bool GetFlag(uint8_t flag);
	void SetFlag(bool byte, uint8_t flag);
	void Reset();
	void AddCycles(int count);
	uint64_t GetCycleCount();
	Bus* bus;
	bool nmiRequested = false;
	void NMI();
	void Activate(bool active);
private:
	void adc(uint8_t operand);
	//void _and(uint8_t operand);
	uint64_t m_cycle_count = 0;
	// Program counter
	int m_pc;
	int m_sp = 0xFD;

	// Registers
	uint8_t m_a;
	uint8_t m_x;
	uint8_t m_y;

	// flags
	uint8_t m_p;
	void _and(uint8_t operand);
	void ASL(uint8_t& byte);
	bool isActive = false;
};