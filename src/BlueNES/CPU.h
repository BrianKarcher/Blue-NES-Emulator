#pragma once
#include <cstdint>
#include <array>
#include <string>

//#define CPUDEBUG
#define NMIDEBUG

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
const uint8_t DEX_IMPLIED = 0xCA;
const uint8_t DEY_IMPLIED = 0x88;
const uint8_t EOR_IMMEDIATE = 0x49;
const uint8_t EOR_ZEROPAGE = 0x45;
const uint8_t EOR_ZEROPAGE_X = 0x55;
const uint8_t EOR_ABSOLUTE = 0x4D;
const uint8_t EOR_ABSOLUTE_X = 0x5D;
const uint8_t EOR_ABSOLUTE_Y = 0x59;
const uint8_t EOR_INDEXEDINDIRECT = 0x41;
const uint8_t EOR_INDIRECTINDEXED = 0x51;
const uint8_t INC_ZEROPAGE = 0xE6;
const uint8_t INC_ZEROPAGE_X = 0xF6;
const uint8_t INC_ABSOLUTE = 0xEE;
const uint8_t INC_ABSOLUTE_X = 0xFE;
const uint8_t INX_IMPLIED = 0xE8;
const uint8_t INY_IMPLIED = 0xC8;
const uint8_t JMP_ABSOLUTE = 0x4C;
const uint8_t JMP_INDIRECT = 0x6C;
const uint8_t JSR_ABSOLUTE = 0x20;
const uint8_t LDA_IMMEDIATE = 0xA9;
const uint8_t LDA_ZEROPAGE = 0xA5;
const uint8_t LDA_ZEROPAGE_X = 0xB5;
const uint8_t LDA_ABSOLUTE = 0xAD;
const uint8_t LDA_ABSOLUTE_X = 0xBD;
const uint8_t LDA_ABSOLUTE_Y = 0xB9;
const uint8_t LDA_INDEXEDINDIRECT = 0xA1;
const uint8_t LDA_INDIRECTINDEXED = 0xB1;
const uint8_t LDX_IMMEDIATE = 0xA2;
const uint8_t LDX_ZEROPAGE = 0xA6;
const uint8_t LDX_ZEROPAGE_Y = 0xB6;
const uint8_t LDX_ABSOLUTE = 0xAE;
const uint8_t LDX_ABSOLUTE_Y = 0xBE;
const uint8_t LDY_IMMEDIATE = 0xA0;
const uint8_t LDY_ZEROPAGE = 0xA4;
const uint8_t LDY_ZEROPAGE_X = 0xB4;
const uint8_t LDY_ABSOLUTE = 0xAC;
const uint8_t LDY_ABSOLUTE_X = 0xBC;
const uint8_t LSR_ACCUMULATOR = 0x4A;
const uint8_t LSR_ZEROPAGE = 0x46;
const uint8_t LSR_ZEROPAGE_X = 0x56;
const uint8_t LSR_ABSOLUTE = 0x4E;
const uint8_t LSR_ABSOLUTE_X = 0x5E;
const uint8_t NOP_IMPLIED = 0xEA;
const uint8_t ORA_IMMEDIATE = 0x09;
const uint8_t ORA_ZEROPAGE = 0x05;
const uint8_t ORA_ZEROPAGE_X = 0x15;
const uint8_t ORA_ABSOLUTE = 0x0D;
const uint8_t ORA_ABSOLUTE_X = 0x1D;
const uint8_t ORA_ABSOLUTE_Y = 0x19;
const uint8_t ORA_INDEXEDINDIRECT = 0x01;
const uint8_t ORA_INDIRECTINDEXED = 0x11;
const uint8_t PHA_IMPLIED = 0x48;
const uint8_t PHP_IMPLIED = 0x08;
const uint8_t PLA_IMPLIED = 0x68;
const uint8_t PLP_IMPLIED = 0x28;
const uint8_t ROL_ACCUMULATOR = 0x2A;
const uint8_t ROL_ZEROPAGE = 0x26;
const uint8_t ROL_ZEROPAGE_X = 0x36;
const uint8_t ROL_ABSOLUTE = 0x2E;
const uint8_t ROL_ABSOLUTE_X = 0x3E;
const uint8_t ROR_ACCUMULATOR = 0x6A;
const uint8_t ROR_ZEROPAGE = 0x66;
const uint8_t ROR_ZEROPAGE_X = 0x76;
const uint8_t ROR_ABSOLUTE = 0x6E;
const uint8_t ROR_ABSOLUTE_X = 0x7E;
const uint8_t RTI_IMPLIED = 0x40;
const uint8_t RTS_IMPLIED = 0x60;
const uint8_t SBC_IMMEDIATE = 0xE9;
const uint8_t SBC_ZEROPAGE = 0xE5;
const uint8_t SBC_ZEROPAGE_X = 0xF5;
const uint8_t SBC_ABSOLUTE = 0xED;
const uint8_t SBC_ABSOLUTE_X = 0xFD;
const uint8_t SBC_ABSOLUTE_Y = 0xF9;
const uint8_t SBC_INDEXEDINDIRECT = 0xE1;
const uint8_t SBC_INDIRECTINDEXED = 0xF1;
const uint8_t SEC_IMPLIED = 0x38;
const uint8_t SED_IMPLIED = 0xF8;
const uint8_t SEI_IMPLIED = 0x78;
const uint8_t STA_ZEROPAGE = 0x85;
const uint8_t STA_ZEROPAGE_X = 0x95;
const uint8_t STA_ABSOLUTE = 0x8D;
const uint8_t STA_ABSOLUTE_X = 0x9D;
const uint8_t STA_ABSOLUTE_Y = 0x99;
const uint8_t STA_INDEXEDINDIRECT = 0x81;
const uint8_t STA_INDIRECTINDEXED = 0x91;
const uint8_t STX_ZEROPAGE = 0x86;
const uint8_t STX_ZEROPAGE_Y = 0x96;
const uint8_t STX_ABSOLUTE = 0x8E;
const uint8_t STY_ZEROPAGE = 0x84;
const uint8_t STY_ZEROPAGE_X = 0x94;
const uint8_t STY_ABSOLUTE = 0x8C;
const uint8_t TAX_IMPLIED = 0xAA;
const uint8_t TAY_IMPLIED = 0xA8;
const uint8_t TSX_IMPLIED = 0xBA;
const uint8_t TXA_IMPLIED = 0x8A;
const uint8_t TXS_IMPLIED = 0x9A;
const uint8_t TYA_IMPLIED = 0x98;

class Bus;

class Processor_6502
{
public:
	Processor_6502();
	void connectBus(Bus* bus);
	void setNMI(bool state);
	void setIRQ(bool state);
	// Power On and Reset are different
	void PowerOn();
	uint8_t Clock();
	uint8_t GetA();
	void SetA(uint8_t a);
	uint8_t GetX();
	void SetX(uint8_t x);
	uint8_t GetY();
	void SetY(uint8_t y);
	bool GetFlag(uint8_t flag);
	void SetFlag(uint8_t flag);
	void Reset();
	void AddCycles(int count);
	uint64_t GetCycleCount();
	Bus* bus;
	bool nmiRequested = false;
	void NMI();
	void Activate(bool active);
	void SetPC(uint16_t address);
	uint16_t GetPC();
	uint8_t GetStatus();
	void SetStatus(uint8_t status);
	void ClearFlag(uint8_t flag);
	uint8_t GetSP();
	void SetSP(uint8_t sp);
	int cyclesThisFrame;
	std::array<std::string, 256> instructionMap;
	void setFrozen(bool frozen) { isFrozen = frozen; }
	void toggleFrozen() { isFrozen = !isFrozen; }
private:
	void push(uint8_t value);
	uint8_t pull();

	// Interrupt lines
	bool nmi_line;
	bool nmi_previous;
	bool nmi_pending;

	bool irq_line;

	void checkInterrupts();
	void handleNMI();
	void handleIRQ();

	void ADC(uint8_t operand);
	//void _and(uint8_t operand);
	uint64_t m_cycle_count = 0;
	// Program counter
	int m_pc;
	uint8_t m_sp = 0xFD;
	inline void dbg(const wchar_t* fmt, ...);
	inline void dbgNmi(const wchar_t* fmt, ...);

	// Registers
	uint8_t m_a;
	uint8_t m_x;
	uint8_t m_y;

	void buildMap();

	// flags
	uint8_t m_p;
	void _and(uint8_t operand);
	void ASL(uint8_t& byte);
	bool NearBranch(uint8_t value);
	void BIT(uint8_t data);
	void cp(uint8_t value, uint8_t operand);
	void EOR(uint8_t operand);
	void LSR(uint8_t& byte);
	void ORA(uint8_t operand);
	void ROL(uint8_t& byte);
	void ROR(uint8_t& byte);
	void SBC(uint8_t operand);
	bool isActive = false;
	inline uint8_t ReadNextByte();
	inline uint8_t ReadNextByte(uint8_t offset);
	inline uint16_t ReadNextWord();
	inline uint16_t ReadNextWord(uint8_t offset);
	inline uint16_t ReadNextWordNoCycle(uint8_t offset);
	inline uint8_t ReadByte(uint16_t addr);
	inline uint16_t ReadIndexedIndirect();
	inline uint16_t ReadIndirectIndexedNoCycle();
	inline void SetZero(uint8_t value);
	inline void SetNegative(uint8_t value);
	inline void SetOverflow(bool condition);
	inline void SetCarry(bool condition);
	inline void SetDecimal(bool condition);
	inline void SetInterrupt(bool condition);
	inline void SetBreak(bool condition);
	inline uint16_t ReadIndirectIndexed();
	int nmiCount = 0;
	//inline void ExtractAbsolute(uint8_t& loByte, uint8_t& hiByte);
	//bool debug = false;

	bool isFrozen = false;
	int count = 0;
};