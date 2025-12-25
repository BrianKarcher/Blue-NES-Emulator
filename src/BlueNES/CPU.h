#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <functional>

//#define CPUDEBUG
//#define NMIDEBUG

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
class OpenBusMapper;
class Serializer;

class CPU
{
public:
	CPU(OpenBusMapper& openBus);
	void connectBus(Bus* bus);

	inline uint8_t ReadNextByte();
	inline uint8_t ReadNextByte(uint8_t offset);
	inline uint16_t ReadNextWord();
	inline uint16_t ReadNextWord(uint8_t offset);
	inline uint16_t ReadNextWordNoCycle(uint8_t offset);
	inline uint8_t ReadByte(uint16_t addr);
	inline uint16_t ReadIndexedIndirect();
	inline uint16_t ReadIndirectIndexedNoCycle();
	void WriteByte(uint16_t addr, uint8_t value);

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
	void ConsumeCycle();

	// Registers
	uint8_t m_a;
	uint8_t m_x;
	uint8_t m_y;
	uint16_t _operand;

	// Internal Latch Registers (to hold state between cycles)
	uint8_t  current_opcode;
	uint8_t  addr_low;
	uint8_t  addr_high;
	uint16_t effective_addr;
	bool inst_complete = true;
	bool addr_complete = false;

	// Emulator State
	int cycle_state;  // Tracks the current micro-op (0, 1, 2...)
	bool page_crossed;

	// The CPU Tick Loop
	void cpu_tick();
	// Helper to update Zero and Negative flags
	void update_ZN_flags(uint8_t value) {
		if (value == 0) m_p |= 0x02; else m_p &= ~0x02; // Zero Flag
		if (value & 0x80) m_p |= 0x80; else m_p &= ~0x80; // Negative Flag
	}

	void init_cpu() {
		// Replace the problematic line with the following code to explicitly use a lambda function:  
		//std::function<InstructionHandler> func = [](CPU& cpu) {
		//	run_instruction<Mode_AbsoluteX<Op_LDA::is_write>, Op_LDA>(cpu);
		//};
		// Or use a direct function pointer assignment:

		//std::function<void(CPU& cpu)> func = &run_instruction<Mode_AbsoluteX<Op_LDA::is_write>, Op_LDA>;

		// $BD = LDA Absolute, X (Read operation)
		// Mode_AbsoluteX takes <false> because LDA::is_write is false
		//opcode_table[0xA5] = &run_instruction<Mode_AbsoluteX<Op_STA::is_write>, Op_STA>;
		opcode_table[0x61] = &run_instruction<Mode_IndirectX, Op_ADC>;
		opcode_table[0x65] = &run_instruction<Mode_ZeroPage, Op_ADC>;
		opcode_table[0x69] = &run_instruction<Mode_Immediate, Op_ADC>;
		opcode_table[0x6D] = &run_instruction<Mode_Absolute, Op_ADC>;
		opcode_table[0x71] = &run_instruction<Mode_IndirectY<Op_ADC::is_rmw>, Op_ADC>;
		opcode_table[0x75] = &run_instruction<Mode_ZeroPageX, Op_ADC>;
		opcode_table[0x79] = &run_instruction<Mode_AbsoluteY<Op_ADC::is_rmw>, Op_ADC>;
		opcode_table[0x7D] = &run_instruction<Mode_AbsoluteX<Op_ADC::is_rmw>, Op_ADC>;
		opcode_table[0x81] = &run_instruction<Mode_IndirectX, Op_STA>;
		opcode_table[0x85] = &run_instruction<Mode_ZeroPage, Op_STA>;
		opcode_table[0x8D] = &run_instruction<Mode_Absolute, Op_STA>;
		opcode_table[0x91] = &run_instruction<Mode_IndirectY<Op_STA::is_rmw>, Op_STA>;
		opcode_table[0x95] = &run_instruction<Mode_ZeroPageX, Op_STA>;
		opcode_table[0x99] = &run_instruction<Mode_AbsoluteY<Op_STA::is_rmw>, Op_STA>;
		opcode_table[0x9D] = &run_instruction<Mode_AbsoluteX<Op_STA::is_rmw>, Op_STA>;
		opcode_table[0xA1] = &run_instruction<Mode_IndirectX, Op_LDA>;
		opcode_table[0xA5] = &run_instruction<Mode_ZeroPage, Op_LDA>;
		opcode_table[0xA9] = &run_instruction<Mode_Immediate, Op_LDA>;
		opcode_table[0xAD] = &run_instruction<Mode_Absolute, Op_LDA>;
		opcode_table[0xB1] = &run_instruction<Mode_IndirectY<Op_LDA::is_rmw>, Op_LDA>;
		opcode_table[0xB5] = &run_instruction<Mode_ZeroPageX, Op_LDA>;
		opcode_table[0xB9] = &run_instruction<Mode_AbsoluteY<Op_LDA::is_rmw>, Op_LDA>;
		opcode_table[0xBD] = &run_instruction<Mode_AbsoluteX<Op_LDA::is_rmw>, Op_LDA>;
		// $FE: INC Absolute, X
		// Uses "is_rmw=true" -> Forces 7 cycles (Mode T1-T3, Op T4-T6)
		opcode_table[0xFE] = &run_instruction<Mode_AbsoluteX<Op_INC::is_rmw>, Op_INC>;
		//opcode_table[0xBD](cpu);

		// $9D = STA Absolute, X (Write operation)
		// This will automatically force the 5th cycle due to is_write=true
		

		// You can reuse Mode_AbsoluteX for ADC, CMP, EOR, etc. easily!
	}

	void Serialize(Serializer& serializer);
	void Deserialize(Serializer& serializer);
private:
	// The effective pattern we are following here is we advance one cycle per read/write to the bus
	// We return false on each cycle until each process is complete.
	// The last cycle in "Mode" returns true on an incomplete cycle
	// with no bus access, so Op runs immediately the same cycle
	// to access the bus and complete the cycle.

	struct Mode_Immediate {
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 1:
				cpu.effective_addr = cpu.m_pc++;

				// No bus access so no cycle is consumed here
				return true;
			}
			return false;
		}
	};

	// Policy: Zero Page (e.g., LDA $nn)
// Cycles: 3 total (T0: Opcode, T1: Fetch Address, T2: Read/Write)
	struct Mode_ZeroPage {
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 1: // T1: Fetch the 8-bit address
				cpu.addr_low = cpu.ReadByte(cpu.m_pc++);
				// High byte is always 0x00 in Zero Page
				cpu.effective_addr = (0x00 << 8) | cpu.addr_low;

				// No page crossing possible, so we are ready for the Op next cycle
				cpu.cycle_state = 2;
				return false;
			case 2:
				return true;
			}
			
			return false;
		}
	};

	// Policy: Zero Page, X (e.g., LDA $nn, X)
	// Cycles: 4 total (T0: Opcode, T1: Fetch Addr, T2: Add X/Dummy Read, T3: Read/Write)
	struct Mode_ZeroPageX {
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
				case 1: // T1: Fetch the base 8-bit address
					cpu.addr_low = cpu.ReadByte(cpu.m_pc++);
					cpu.cycle_state = 2;
					return false;

				case 2: // T2: Add X and perform a Dummy Read
					// Note: The 6502 hardware does a read here at the unindexed address
					cpu.ReadByte((0x00 << 8) | cpu.addr_low);

					// Add X and FORCE wrap within page zero (using & 0xFF)
					cpu.effective_addr = (0x00 << 8) | ((cpu.addr_low + cpu.m_x) & 0xFF);

					// Ready for the Op next cycle
					cpu.cycle_state = 3;
					return false;

				case 3:
					return true;
			}
			return false;
		}
	};

	// Policy: Absolute Addressing Mode ($aaaa)
	// Usage: LDA $1234, STA $4000, etc.
	struct Mode_Absolute {
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 1: // T1: Fetch Low Byte of Address
				cpu.addr_low = cpu.ReadByte(cpu.m_pc++);
				cpu.cycle_state = 2;
				return false;

			case 2: // T2: Fetch High Byte of Address
				cpu.addr_high = cpu.ReadByte(cpu.m_pc++);

				// Combine into the 16-bit effective address
				cpu.effective_addr = (cpu.addr_high << 8) | cpu.addr_low;

				// In Absolute mode, the address is now fully formed.
				// There is no addition, so no T3 penalty/dummy read is needed.
				cpu.cycle_state = 3;
				return false;
			case 3:
				return true;
			}
			return false;
		}
	};

	// Policy: Absolute X Addressing Mode
	// Template param 'AlwaysPenalty' is true for STA, INC, DEC, ASL, etc.
	template <bool always_penalty>
	struct Mode_AbsoluteX {
		// Returns true when the addressing sequence is totally finished
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 1: // Fetch Low
				cpu.addr_low = cpu.ReadByte(cpu.m_pc++);
				cpu.cycle_state = 2;
				return false;

			case 2: // Fetch High & Calc
			{
				cpu.addr_high = cpu.ReadByte(cpu.m_pc++);

				// Calculate effective address (wrapping low byte)
				cpu.effective_addr = (cpu.addr_high << 8) | ((cpu.addr_low + cpu.m_x) & 0xFF);

				// Check page crossing
				cpu.page_crossed = ((uint16_t)cpu.addr_low + cpu.m_x) > 0xFF;

				cpu.cycle_state = 3;
				return false;
			}

			case 3: // Potential Dummy Read / Fix Up
				// If it's a WRITE or we crossed a page, we generally strictly need the fixup
				// For READs: if page crossed, we read garbage (dummy), then fix.
				// For READs: if NO cross, we are actually DONE with addressing.
				
				if (always_penalty || cpu.page_crossed) {
					// Do the dummy read (hardware does this)
					cpu.ReadByte(cpu.effective_addr);
					if (cpu.page_crossed) {
						// Fix the high byte for the next cycle
						cpu.effective_addr = ((cpu.addr_high + 1) << 8) | ((cpu.addr_low + cpu.m_x) & 0xFF);
					}
					cpu.cycle_state = 4;
					return false;
				}
				// We are done.
				return true;
			case 4:
				// Address is now fixed. Ready to execute.
				return true;
			}
			return false;
		}
	};

	// Policy: Absolute Y Addressing Mode
	// Template param 'AlwaysPenalty' is true for STA, INC, DEC, ASL, etc.
	template <bool always_penalty>
	struct Mode_AbsoluteY {
		// Returns true when the addressing sequence is totally finished
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 1: // Fetch Low
				cpu.addr_low = cpu.ReadByte(cpu.m_pc++);
				cpu.cycle_state = 2;
				return false;

			case 2: // Fetch High & Calc
			{
				cpu.addr_high = cpu.ReadByte(cpu.m_pc++);

				// Calculate effective address (wrapping low byte)
				cpu.effective_addr = (cpu.addr_high << 8) | ((cpu.addr_low + cpu.m_y) & 0xFF);

				// Check page crossing
				cpu.page_crossed = ((uint16_t)cpu.addr_low + cpu.m_y) > 0xFF;

				cpu.cycle_state = 3;
				return false;
			}

			case 3: // Potential Dummy Read / Fix Up
				// If it's a WRITE or we crossed a page, we generally strictly need the fixup
				// For READs: if page crossed, we read garbage (dummy), then fix.
				// For READs: if NO cross, we are actually DONE with addressing.

				if (always_penalty || cpu.page_crossed) {
					// Do the dummy read (hardware does this)
					cpu.ReadByte(cpu.effective_addr);
					if (cpu.page_crossed) {
						// Fix the high byte for the next cycle
						cpu.effective_addr = ((cpu.addr_high + 1) << 8) | ((cpu.addr_low + cpu.m_y) & 0xFF);
					}
					cpu.cycle_state = 4;
					return false;
				}
				// We are done.
				return true;
			case 4:
				// Address is now fixed. Ready to execute.
				return true;
			}
			return false;
		}
	};

	// Policy: Indexed Indirect X Addressing Mode ($nn, X)
	// Usage: LDA ($nn, X)
	// This mode is always a fixed length (6 cycles total).
	struct Mode_IndirectX {
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
				case 1: // Fetch Zero Page Base Address
					// Read the immediate byte (base address in ZP)
					cpu.addr_low = cpu.ReadByte(cpu.m_pc++);
					cpu.cycle_state = 2;
					return false;

				case 2: // T2: Addition / Dummy Read
					// Hardware adds X to the base address. 
					// It performs a dummy read at the base address.
					cpu.ReadByte(cpu.addr_low);

					// Calculate the actual pointer location (wrapped in ZP)
					cpu.addr_low = (cpu.addr_low + cpu.m_x) & 0xFF;
					cpu.cycle_state = 3;
					return false;

				case 3: // T3: Fetch Effective Address Low
					// Read the low byte of the final address from ZP
					cpu.effective_addr = cpu.ReadByte(cpu.addr_low);
					cpu.cycle_state = 4;
					return false;

				case 4: // T4: Fetch Effective Address High
					// Read the high byte of the final address from ZP+1
					// Must use & 0xFF to ensure ZP wrap-around (FF -> 00)
					{
						uint8_t high = cpu.ReadByte((cpu.addr_low + 1) & 0xFF);
						cpu.effective_addr |= (high << 8);
					}

					// The address is now fully formed.
					// In the 6502 architecture, this mode always completes the 
					// addressing phase here. The Op runs in T5.
					cpu.cycle_state = 5;
					return false;
				case 5:
					return true;
			}
			return false;
		}
	};

	// Policy: Indirect Indexed Y Addressing Mode ($nn), Y
	// Usage: LDA ($nn), Y etc.
	template <bool always_penalty>
	struct Mode_IndirectY {
		// Returns true when the addressing sequence is totally finished
		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 1: // Fetch Pointer Address (from Zero Page)
				// We temporarily use effective_addr to hold the ZP pointer address
				cpu.effective_addr = cpu.ReadByte(cpu.m_pc++);
				cpu.cycle_state = 2;
				return false;

			case 2: // Fetch Effective Address Low Byte
				// Read the low byte of the pointer from ZP
				cpu.addr_low = cpu.ReadByte(cpu.effective_addr);
				cpu.cycle_state = 3;
				return false;

			case 3: // Fetch Effective Address High Byte & Add Y
			{
				// Read the high byte of the pointer from ZP+1 (WITH ZP WRAP!)
				cpu.addr_high = cpu.ReadByte((cpu.effective_addr + 1) & 0xFF);

				// Now we calculate the "tentative" effective address (without carry fix)
				// effective_addr = (Pointer_High << 8) | ((Pointer_Low + Y) & 0xFF)
				cpu.effective_addr = (cpu.addr_high << 8) | ((cpu.addr_low + cpu.m_y) & 0xFF);

				// Check if adding Y crossed a page boundary
				cpu.page_crossed = ((uint16_t)cpu.addr_low + cpu.m_y) > 0xFF;

				cpu.cycle_state = 4;
				return false;
			}

			case 4: // Potential Dummy Read / Fix Up

				if (always_penalty || cpu.page_crossed) {
					// Dummy Read from the "wrong" page (bits 0-7 correct, bits 8-15 wrong)
					cpu.ReadByte(cpu.effective_addr);

					// Fix the high byte for the next cycle
					// Address = ((Pointer_High + 1) << 8) | (Pointer_Low + Y)
					if (cpu.page_crossed) {
						cpu.effective_addr = ((cpu.addr_high + 1) << 8) | ((cpu.addr_low + cpu.m_y) & 0xFF);
					}

					cpu.cycle_state = 5;
					return false;
				}

				// Optimization: If Read & No Cross, we are done.
				// Op will run immediately in this same tick (T4).
				return true;

			case 5:
				// Address is fixed. Ready to execute (T5).
				return true;
			}
			return false;
		}
	};

	struct Op_ADC {
		static constexpr bool is_rmw = false;

		static bool step(CPU& cpu) {
			cpu._operand = cpu.ReadByte(cpu.effective_addr);
			uint8_t a_old = cpu.m_a;

			uint16_t result = cpu.m_a + cpu._operand + (cpu.m_p & FLAG_CARRY ? 1 : 0);
			cpu.m_a = result & 0xFF;  // Update accumulator with low byte
			// Set/clear carry flag
			if (result > 0xFF) {
				cpu.m_p |= FLAG_CARRY;   // Set carry
			}
			else {
				cpu.m_p &= ~FLAG_CARRY;  // Clear carry
			}
			// Set/clear overflow flag
			// Overflow occurs when:
			// - Adding two positive numbers results in a negative number, OR
			// - Adding two negative numbers results in a positive number
			// This can be detected by checking if the sign bits of both operands
			// are the same, but different from the result's sign bit
			if (((a_old ^ cpu.m_a) & (cpu._operand ^ cpu.m_a) & 0x80) != 0) {
				cpu.m_p |= FLAG_OVERFLOW;   // Set overflow
			}
			else {
				cpu.m_p &= ~FLAG_OVERFLOW;  // Clear overflow
			}
			// Set/clear zero flag
			if (cpu.m_a == 0) {
				cpu.m_p |= FLAG_ZERO;
			}
			else {
				cpu.m_p &= ~FLAG_ZERO;
			}

			// Set/clear negative flag (bit 7 of result)
			if (cpu.m_a & 0x80) {
				cpu.m_p |= FLAG_NEGATIVE;
			}
			else {
				cpu.m_p &= ~FLAG_NEGATIVE;
			}
			return true;
		}
	};

	// 3 Cycle RMW Execution (Reuses logic structure of INC!)
	struct Op_DEC {
		static constexpr bool is_rmw = true;

		static bool step(CPU& cpu) {
			switch (cpu.cycle_state) {
			case 0: // Read
				cpu._operand = cpu.ReadByte(cpu.effective_addr);
				cpu.cycle_state = 1;
				return false;
			case 1: // Dummy Write
				cpu.WriteByte(cpu.effective_addr, cpu._operand);
				cpu.cycle_state = 2;
				return false;
			case 2: // Modify & Write
				cpu._operand--; // DECREMENT
				cpu.update_ZN_flags(cpu._operand);
				cpu.WriteByte(cpu.effective_addr, cpu._operand);
				return true;
			}
			return false;
		}
	};

	struct Op_INC {
		// Defines that this Op requires the "Always Penalty" path in addressing
		static constexpr bool is_rmw = true;

		static bool step(CPU& cpu) {
			// cycle_state continues incrementing from where Mode left off
			switch (cpu.cycle_state) {
			case 0: // T4: Read Real Value
				cpu._operand = cpu.ReadByte(cpu.effective_addr);
				cpu.cycle_state = 1;
				return false;

			case 1: // T5: Dummy Write (Write original value back)
				// 6502 quirk: It writes the unmodified value while ALU works
				cpu.WriteByte(cpu.effective_addr, cpu._operand);
				cpu.cycle_state = 2;
				return false;

			case 2: // T6: Final Write (Write modified value)
				cpu._operand++; // The actual increment
				cpu.update_ZN_flags(cpu._operand);
				cpu.WriteByte(cpu.effective_addr, cpu._operand);
				return true; // Instruction Complete
			}
			return false;
		}
	};

	struct Op_LDA {
		static bool step(CPU& cpu) {
			cpu.m_a = cpu.ReadByte(cpu.effective_addr);
			cpu.update_ZN_flags(cpu.m_a);
			return true;
		}
		static constexpr bool is_rmw = false; // Trait used by the Addressing Mode
	};

	struct Op_STA {
		static bool step(CPU& cpu) {
			cpu.WriteByte(cpu.effective_addr, cpu.m_a);
			//bus_write(cpu.effective_addr, cpu.A);
			return true;
		}
		static constexpr bool is_rmw = true;
	};

	template <typename Mode, typename Op>
	static void run_instruction(CPU& cpu) {
		// 1. Run the Addressing Mode logic
		if (!cpu.addr_complete) {
			// Run Addressing Mode logic
			// Returns TRUE when the effective_addr is ready on the bus
			cpu.addr_complete = Mode::step(cpu);
			if (cpu.addr_complete) {
				// Reset cycle state for Op execution
				cpu.cycle_state = 0;
			}
		}
		if (cpu.addr_complete) {
			// See notes way above on why we run the op immediately after addressing completes
			// Run Operation logic
			// Returns TRUE when the instruction is fully complete
			bool finished = Op::step(cpu);
			if (finished) {
				// Reset state for next opcode
				cpu.inst_complete = true;
				cpu.cycle_state = 0;
				cpu.addr_complete = false;
				// Trigger fetch of next opcode here or via main loop
			}
		}
	}

	// Define a function pointer type for our micro-op handlers
	typedef void (*InstructionHandler)(CPU&);

	// Template function
	//template <typename T>
	//T multiply(const T& a, const T& b) {
	//	return a * b;
	//}

	//void LDA_AbsX(CPU& cpu) {
	//	run_instruction<Mode_AbsoluteX<Op_LDA::is_write>, Op_LDA>(cpu);
	//}

	//void LDA_ZP(CPU& cpu) {
	//	run_instruction<Mode_AbsoluteX<Op_LDA::is_write>, Op_LDA>(cpu);
	//	run_instruction<Mode_ZeroPage<Op_LDA::is_write>, Op_LDA, Op_LDA>(cpu);
	//}

	// The Lookup Table
	InstructionHandler opcode_table[256]; // = { &LDA_AbsX };
    
  //  void init_cpu() {
		////opcode_table[0xBD] = &LDA_AbsX;

  //      // Use a lambda with explicit capture of 'this' to fix the error  
  //      //(*opcode_table[0xBD])(CPU&) = [this](CPU& cpu) {
  //      //    this->run_instruction<Mode_AbsoluteX<Op_LDA::is_write>, Op_LDA>(cpu);  
  //      //};  

  //      //opcode_table[0x9D] = [this](CPU& cpu) {  
  //      //    this->run_instruction<Mode_AbsoluteX<Op_STA::is_write>, Op_STA>(cpu);  
  //      //};  
  //  }

	typedef void (CPU::* OpFunc)(void);
	OpFunc _opcodeTable[256] = {
		//      0          1		  2          3          4          5          6          7          8		   9          A          B          C          D          E          F
		&CPU::BRK,& CPU::ORA,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::ORA,& CPU::ASL,& CPU::DMP,& CPU::PHP,& CPU::ORA,& CPU::ASL,& CPU::DMP,& CPU::DMP,& CPU::ORA,& CPU::ASL, &CPU::DMP, // 0
		&CPU::BPL,& CPU::ORA,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::ORA,& CPU::ASL,& CPU::DMP,& CPU::CLC,& CPU::ORA,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::ORA,& CPU::ASL,& CPU::DMP, // 1
		&CPU::JSR,& CPU::AND,& CPU::DMP,& CPU::DMP,& CPU::BIT,& CPU::AND,& CPU::ROL,& CPU::DMP,& CPU::PLP,& CPU::AND,& CPU::ROL,& CPU::DMP,& CPU::BIT,& CPU::AND,& CPU::ROL,& CPU::DMP, // 2
		&CPU::BMI,& CPU::AND,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::AND,& CPU::ROL,& CPU::DMP,& CPU::SEC,& CPU::AND,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::AND,& CPU::ROL,& CPU::DMP, // 3
		&CPU::RTI,& CPU::EOR,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::EOR,& CPU::LSR,& CPU::DMP,& CPU::PHA,& CPU::EOR,& CPU::LSR,& CPU::DMP,& CPU::JMP_ABS,& CPU::EOR,& CPU::LSR,& CPU::DMP, // 4
		&CPU::BVC,& CPU::EOR,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::EOR,& CPU::LSR,& CPU::DMP,& CPU::CLI,& CPU::EOR,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::EOR,& CPU::LSR,& CPU::DMP, // 5
		&CPU::RTS,& CPU::ADC,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::ADC,& CPU::ROR,& CPU::DMP,& CPU::PLA,& CPU::ADC,& CPU::ROR,& CPU::DMP,& CPU::JMP_IND,& CPU::ADC,& CPU::ROR,& CPU::DMP, // 6
		&CPU::BVS,& CPU::ADC,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::ADC,& CPU::ROR,& CPU::DMP,& CPU::SEI,& CPU::ADC,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::ADC,& CPU::ROR,& CPU::DMP, // 7
		&CPU::DMP,& CPU::STA,& CPU::DMP,& CPU::DMP,& CPU::STY,& CPU::STA,& CPU::STX,& CPU::DMP,& CPU::DEY,& CPU::DMP,& CPU::TXA,& CPU::DMP,& CPU::STY,& CPU::STA,& CPU::STX,& CPU::DMP, // 8
		&CPU::BCC,& CPU::STA,& CPU::DMP,& CPU::DMP,& CPU::STY,& CPU::STA,& CPU::STX,& CPU::DMP,& CPU::TYA,& CPU::STA,& CPU::TXS,& CPU::DMP,& CPU::DMP,& CPU::STA,& CPU::DMP,& CPU::DMP, // 9
		&CPU::LDY,& CPU::LDA,& CPU::LDX,& CPU::DMP,& CPU::LDY,& CPU::LDA,& CPU::LDX,& CPU::DMP,& CPU::TAY,& CPU::LDA,& CPU::TAX,& CPU::DMP,& CPU::LDY,& CPU::LDA,& CPU::LDX,& CPU::DMP, // A
		&CPU::BCS,& CPU::LDA,& CPU::DMP,& CPU::DMP,& CPU::LDY,& CPU::LDA,& CPU::LDX,& CPU::DMP,& CPU::CLV,& CPU::LDA,& CPU::TSX,& CPU::DMP,& CPU::LDY,& CPU::LDA,& CPU::LDX,& CPU::DMP, // B
		&CPU::CPY,& CPU::CMP,& CPU::DMP,& CPU::DMP,& CPU::CPY,& CPU::CMP,& CPU::DEC,& CPU::DMP,& CPU::INY,& CPU::CMP,& CPU::DEX,& CPU::DMP,& CPU::CPY,& CPU::CMP,& CPU::DEC,& CPU::DMP, // C
		&CPU::BNE,& CPU::CMP,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::CMP,& CPU::DEC,& CPU::DMP,& CPU::CLD,& CPU::CMP,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::CMP,& CPU::DEC,& CPU::DMP, // D
		&CPU::CPX,& CPU::SBC,& CPU::DMP,& CPU::DMP,& CPU::CPX,& CPU::SBC,& CPU::INC,& CPU::DMP,& CPU::INX,& CPU::SBC,& CPU::NOP,& CPU::DMP,& CPU::CPX,& CPU::SBC,& CPU::INC,& CPU::DMP, // E
		&CPU::BEQ,& CPU::SBC,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::SBC,& CPU::INC,& CPU::DMP,& CPU::SED,& CPU::SBC,& CPU::DMP,& CPU::DMP,& CPU::DMP,& CPU::SBC,& CPU::INC,& CPU::DMP  // F
	};

	enum AddressingMode {
		ACC,
		IMP,
		IMM,
		ZP,
		ZPX,
		ZPY,
		ABS,
		ABSX,
		ABSY,
		IND,
		INDX,
		INDY,
		REL,
		NONE
	};

	AddressingMode instMode[256] = {
		// 0     1	    2     3      4      5     6    7     8     9       A     B      C      D     E     F
		   IMP,  INDX, NONE, NONE,  NONE,  ZP,   ZP,  NONE, IMP,  IMM,    ACC,  NONE,  NONE,  ABS,  ABS,  NONE, // 0
		   REL,  INDY, NONE, NONE,  NONE,  ZPX,  ZPX, NONE, IMP,  ABSY,   NONE, NONE,  NONE,  ABSX, ABSX, NONE, // 1
		   ABS,  INDX, NONE, NONE,  ZP,    ZP,   ZP,  NONE, IMP,  IMM,    ACC,  NONE,  ABS,   ABS,  ABS,  NONE, // 2
		   REL,  INDY, NONE, NONE,  NONE,  ZPX,  ZPX, NONE, IMP,  ABSY,   NONE, NONE,  NONE,  ABSX, ABSX, NONE, // 3
		   IMP,  INDX, NONE, NONE,  NONE,  ZP,   ZP,  NONE, IMP,  IMM,    ACC,  NONE,  ABS,   ABS,  ABS,  NONE, // 4
		   REL,  INDY, NONE, NONE,  NONE,  ZPX,  ZPX, NONE, IMP,  ABSY,   NONE, NONE,  NONE,  ABSX, ABSX, NONE, // 5
		   IMP,  INDX, NONE, NONE,  NONE,  ZP,   ZP,  NONE, IMP,  IMM,    ACC,  NONE,  IND,   ABS,  ABS,  NONE, // 6
		   REL,  INDY, NONE, NONE,  NONE,  ZPX,  ZPX, NONE, IMP,  ABSY,   NONE, NONE,  NONE,  ABSX, ABSX, NONE, // 7
		   NONE, INDX, NONE, NONE,  ZP,    ZP,   ZP,  NONE, IMP,  NONE,   IMP,  NONE,  ABS,   ABS,  ABS,  NONE, // 8
		   REL,  INDY, NONE, NONE,  ZPX,   ZPX,  ZPY, NONE, IMP,  ABSY,   IMP,  NONE,  NONE,  ABSX, NONE, NONE, // 9
		   IMM,  INDX, IMM,  NONE,  ZP,    ZP,   ZP,  NONE, IMP,  IMM,    IMP,  NONE,  ABS,   ABS,  ABS,  NONE, // A
		   REL,  INDY, NONE, NONE,  ZPX,   ZPX,  ZPY, NONE, IMP,  ABSY,   IMP,  NONE,  ABSX,  ABSX, ABSY, NONE, // B
		   IMM,  INDX, NONE, NONE,  ZP,    ZP,   ZP,  NONE, IMP,  IMM,    IMP,  NONE,  ABS,   ABS,  ABS,  NONE, // C
		   REL,  INDY, NONE, NONE,  NONE,  ZPX,  ZPX, NONE, IMP,  ABSY,   NONE, NONE,  NONE,  ABSX, ABSX, NONE, // D
		   IMM,  INDX, NONE, NONE,  ZP,    ZP,   ZP,  NONE, IMP,  IMM,    IMP,  NONE,  ABS,   ABS,  ABS,  NONE, // E
		   REL,  INDY, NONE, NONE,  NONE,  ZPX,  ZPX, NONE, IMP,  ABSY,   NONE, NONE,  NONE,  ABSX, ABSX, NONE, // F
	};

	OpenBusMapper& openBus;
	void push(uint8_t value);
	uint8_t pull();

	// Interrupt lines
	bool nmi_line;
	bool nmi_previous;
	bool nmi_pending;

	bool prev_irq_line;
	bool irq_line;

	int checkInterrupts();
	void handleNMI();
	void handleIRQ();

	
	//void _and(uint8_t operand);
	uint64_t m_cycle_count = 0;
	// Program counter
	int m_pc;
	uint8_t m_sp = 0xFD;
	inline void dbg(const wchar_t* fmt, ...);
	inline void dbgNmi(const wchar_t* fmt, ...);

	void buildMap();

	// flags
	uint8_t m_p;
	void ADC();
	void AND();
	void ASL();
	void BCC();
	void BCS();
	void BEQ();
	void BMI();
	void BRK();
	bool NearBranch(uint8_t value);
	void BIT();
	void BNE();
	void BPL();
	void BVC();
	void BVS();
	void CLC();
	void CLD();
	void CLI();
	void CLV();
	void CMP();
	void cp(uint8_t value, uint8_t operand);
	void CPX();
	void CPY();
	void DEC();
	void DEX();
	void DEY();
	void DMP();
	void EOR();
	void INC();
	void INX();
	void INY();
	void JMP_ABS() {
		JMP(ReadNextWord());
		dbg(L"0x%04X", m_pc);
		m_cycle_count += 3;
	}
	void JMP_IND() {
		uint16_t addr = ReadNextWord();
		uint8_t loByte = ReadByte(addr);
		// Simulate page boundary hardware bug
		uint8_t hiByte = ReadByte((addr & 0xFF00) | ((addr + 1) & 0x00FF));
		JMP((hiByte << 8) | loByte);
		dbg(L"0x%04X", m_pc);
		dbg(L"0x%04X 0x%02X 0x%02X", addr, loByte, hiByte);
		m_cycle_count += 5;
	}
	void JMP(uint16_t addr);
	void JSR();
	void LDA();
	void LDX();
	void LDY();
	void LSR();
	void NOP();
	void ORA();
	void PHA();
	void PHP();
	void PLA();
	void PLP();
	void ROL();
	void ROR();
	void RTI();
	void RTS();
	void SBC();
	void SEC();
	void SED();
	void SEI();
	void STA();
	void STX();
	void STY();
	void TAX();
	void TAY();
	void TSX();
	void TXA();
	void TXS();
	void TYA();

	bool isActive = false;

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



