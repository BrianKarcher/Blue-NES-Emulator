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
const uint8_t ASL_ABSOLUTE = 0x0E;
const uint8_t ASL_ABSOLUTE_X = 0x1E;
const uint8_t BCC_RELATIVE = 0x90;
const uint8_t BCS_RELATIVE = 0xB0;
const uint8_t BEQ_RELATIVE = 0xF0;
const uint8_t BIT_ZEROPAGE = 0x24;
const uint8_t BIT_ABSOLUTE = 0x2C;
const uint8_t BMI_RELATIVE = 0x30;
const uint8_t BNE_RELATIVE = 0xD0;
const uint8_t BPL_RELATIVE = 0x10;
const uint8_t BRK_IMPLIED = 0x00;
const uint8_t BVC_RELATIVE = 0x50;
const uint8_t BVS_RELATIVE = 0x70;
const uint8_t CLC_IMPLIED = 0x18;
const uint8_t CLD_IMPLIED = 0xD8;
const uint8_t CLI_IMPLIED = 0x58;
const uint8_t CLV_IMPLIED = 0xB8;
const uint8_t CMP_IMMEDIATE = 0xC9;
const uint8_t CMP_ZEROPAGE = 0xC5;
const uint8_t CMP_ZEROPAGE_X = 0xD5;
const uint8_t CMP_ABSOLUTE = 0xCD;
const uint8_t CMP_ABSOLUTE_X = 0xDD;
const uint8_t CMP_ABSOLUTE_Y = 0xD9;
const uint8_t CMP_INDEXEDINDIRECT = 0xC1;
const uint8_t CMP_INDIRECTINDEXED = 0xD1;
const uint8_t CPX_IMMEDIATE = 0xE0;
const uint8_t CPX_ZEROPAGE = 0xE4;
const uint8_t CPX_ABSOLUTE = 0xEC;
const uint8_t CPY_IMMEDIATE = 0xC0;
const uint8_t CPY_ZEROPAGE = 0xC4;
const uint8_t CPY_ABSOLUTE = 0xCC;
const uint8_t DEC_ZEROPAGE = 0xC6;
const uint8_t DEC_ZEROPAGE_X = 0xD6;
const uint8_t DEC_ABSOLUTE = 0xCE;
const uint8_t DEC_ABSOLUTE_X = 0xDE;

const uint8_t NOP_IMPLIED = 0xEA;


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
	void SetPC(uint16_t address);
	uint16_t GetPC();
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
	bool NearBranch(uint8_t value);
	void BIT(uint8_t data);
	void cp(uint8_t value, uint8_t operand);
	bool isActive = false;
};