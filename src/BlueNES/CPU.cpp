#include "CPU.h"
#include "Bus.h"
#include <Windows.h>
#include "PPU.h"
#include "RendererLoopy.h"
#include "OpenBusMapper.h"
#include "Serializer.h"

CPU::CPU(OpenBusMapper& openBus) : openBus(openBus) {
	buildMap();
}

void CPU::connectBus(Bus* bus) {
	this->bus = bus;
}

void CPU::cpu_tick() {
	if (inst_complete) {
		// Priority 1: NMI
		if (nmi_line) {
			// Hardware puts PC on address bus but ignores the data returned,
			// then prepares the NMI sequence.
			ReadByte(m_pc);
			current_opcode = OP_NMI;
			nmi_line = false;
		}
		// Priority 2: IRQ
		else if (irq_line && GetFlag(FLAG_INTERRUPT) == 0) {
			ReadByte(m_pc);
			current_opcode = OP_IRQ;
		}
		// Priority 3: Normal Fetch
		else {
			// This is the actual T0 Read.
			current_opcode = ReadByte(m_pc++);
		}
		cycle_state = 1;
		inst_complete = false;
	}
	else {
		// Execute the micro-op for the current instruction
		opcode_table[current_opcode](*this);
	}
	m_cycle_count++;
}

/* We emulate the 6502 only as far as it is compatible with the NES. For example, we do not include Decimal Mode.*/

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html
int cnt = 0;
int cnt2 = 0;

void CPU::Serialize(Serializer& serializer) {
	CPUState cpu;
	cpu.m_a = m_a;
	cpu.m_x = m_x;
	cpu.m_y = m_y;
	cpu.m_pc = m_pc;
	cpu.m_sp = m_sp;
	cpu.m_p = m_p;
	cpu.nmi_line = nmi_line;
	cpu.nmi_previous = nmi_previous;
	cpu.nmi_pending = nmi_pending;
	cpu.irq_line = irq_line;
	serializer.Write(cpu);
}

void CPU::Deserialize(Serializer& serializer) {
	CPUState cpu;
	serializer.Read(cpu);
	m_a = cpu.m_a;
	m_x = cpu.m_x;
	m_y = cpu.m_y;
	m_pc = cpu.m_pc;
	m_sp = cpu.m_sp;
	m_p = cpu.m_p;
	nmi_line = cpu.nmi_line;
	nmi_previous = cpu.nmi_previous;
	nmi_pending = cpu.nmi_pending;
	irq_line = cpu.irq_line;
}

// ---------------- Debug helper ----------------
inline void CPU::dbg(const wchar_t* fmt, ...) {
#ifdef CPUDEBUG
	//if (!debug) return;
	wchar_t buf[512];
	va_list args;
	va_start(args, fmt);
	_vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
	va_end(args);
	OutputDebugStringW(buf);
#endif
}

inline void CPU::dbgNmi(const wchar_t* fmt, ...) {
#ifdef NMIDEBUG
	//if (!debug) return;
	wchar_t buf[512];
	va_list args;
	va_start(args, fmt);
	_vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
	va_end(args);
	OutputDebugStringW(buf);
#endif
}

void CPU::PowerOn()
{
	m_pc = 0xFFFC;
	uint8_t resetLo = ReadNextByte();
	m_pc = 0xFFFD;
	uint8_t resHi = ReadNextByte();
	m_pc = (static_cast<uint16_t>(resHi << 8)) | resetLo;
	m_p = 0;
	m_a = 0;
	m_x = 0;
	m_y = 0;
	m_sp = 0xFD;
	m_cycle_count = 0;
	isActive = true;
	nmi_line = false;
	nmi_previous = false;
	irq_line = false;
	nmi_pending = false;
	isFrozen = false;
}

void CPU::Activate(bool active)
{
	isActive = active;
}

void CPU::Reset()
{
	// TODO : Add the reset vector read from the bus
	m_pc = (static_cast<uint16_t>(bus->read(0xFFFD) << 8)) | bus->read(0xFFFC);
	m_a = 0;
	m_x = 0;
	m_y = 0;
	m_p = 0x24; // Set unused flag bit to 1, others to 0
	m_sp = 0xFD;
	m_cycle_count = 0;
	nmi_line = false;
	nmi_previous = false;
	irq_line = false;
	nmi_pending = false;
	isFrozen = false;
}

void CPU::AddCycles(int count)
{
	m_cycle_count += count;
}

uint64_t CPU::GetCycleCount()
{
	return m_cycle_count;
}

void CPU::handleNMI() {
	if (isFrozen) {
		return;
	}
	// Push PC and P to stack
	nmiCount++;
	if (nmiCount == 6) {
		int i = 0;
	}
	dbgNmi(L"\nTaking NMI (%d) (cycle %d)\n", nmiCount, m_cycle_count);
	//dbgNmi(L"Writing 0x%02X to stack 0x%02X\n", (m_pc >> 8) & 0xFF, m_sp);
	bus->write(0x0100 + m_sp--, (m_pc >> 8) & 0xFF); // Push high byte of PC
	//dbgNmi(L"Writing 0x%02X to stack 0x%02X\n", m_pc & 0xFF, m_sp);
	bus->write(0x0100 + m_sp--, m_pc & 0xFF);        // Push low byte of PC
	//dbgNmi(L"Writing 0x%02X to stack 0x%02X\n", m_p, m_sp);
	bus->write(0x0100 + m_sp--, m_p);                 // Push processor status
	// THIS MUST BE DONE AFTER PUSHING P TO STACK. OTHERWISE WILL BREAK MMC3 FOREVER!
	// Technically it is because RTI pulls its state from the stack, and that state needs
	// to have the interrupt flag set to whatever it was PRIOR to the NMI occurring.
	// Since, you know, that is where you are RETURNING TO.
	// Yes, this bug pissed me off.
	m_p |= FLAG_INTERRUPT;
	// Set PC to NMI vector
	m_pc = (static_cast<uint16_t>(bus->read(0xFFFB) << 8)) | bus->read(0xFFFA);
	//dbgNmi(L"pc set to 0x%04X\n", m_pc);
	//dbgNmi(L"Stack set to ");
	//for (uint8_t i = m_sp + 1; i != 0; i++) {
	//	dbgNmi(L"0x%02X ", bus->read(0x100 + i));
	//}
	// NMI takes 7 cycles
	m_cycle_count += 7;
	count += 1;
	//if (count == 2) {
	//	isFrozen = true;
	//}
}

// Called by mappers/devices to set interrupt lines
void CPU::setNMI(bool state) {
	nmi_line = state;
}

void CPU::setIRQ(bool state) {
	irq_line = state;
}

void CPU::push(uint8_t value) {
	bus->write(0x0100 + m_sp, value);
	m_sp--;
}

uint8_t CPU::pull() {
	m_sp++;
	return bus->read(0x0100 + m_sp);
}

void CPU::handleIRQ() {
	// IRQ takes 7 cycles

	// Push PC (high byte first)
	push((m_pc >> 8) & 0xFF);
	push(m_pc & 0xFF);

	// Push status register
	// Bit 5 (unused) is always set
	// Bit 4 (B flag) is clear for interrupts
	push((m_p & ~FLAG_BREAK) | FLAG_UNUSED);

	// Set interrupt disable flag
	m_p |= FLAG_INTERRUPT;

	// Load IRQ vector from $FFFE-$FFFF
	uint8_t low = ReadByte(0xFFFE);
	uint8_t high = ReadByte(0xFFFF);
	m_pc = (high << 8) | low;

	m_cycle_count += 7;
}

int CPU::checkInterrupts() {
	// NMI - Edge-triggered (detects 0->1 transition)
	if (nmi_line && !nmi_previous) {
		nmi_pending = true;
	}
	nmi_previous = nmi_line;

	// Handle NMI (higher priority than IRQ)
	if (nmi_pending) {
		handleNMI();
		nmi_pending = false;
		return 7;  // Don't check IRQ if NMI occurred
	}

	// IRQ - Level-triggered, maskable by I flag
	// Check if IRQ line is active AND hardware interrupts are enabled
	if (irq_line && !(m_p & FLAG_INTERRUPT)) {
	//if (irq_line) {
		handleIRQ();
		irq_line = false; // Clear IRQ line after handling
		return 7;
	}
	// "it's really the status of the interrupt lines at the end of the second-to-last cycle that matters."
	//irq_line = bus->IrqPending();
	prev_irq_line = irq_line;

	return 0;
}

/// <summary>
/// Speed is paramount so we try to reduce function hops as much as feasible while keeping the code readable.
/// </summary>
uint8_t CPU::Clock()
{
	if (!isActive) return 0;

	uint16_t current_pc = m_pc;
	uint8_t op = ReadNextByte();
	uint64_t cyclesBefore = m_cycle_count;
	dbg(L"\n(%d) 0x%04X %S ", cyclesBefore, current_pc, instructionMap[op].c_str());
	if (m_cycle_count > 1102773) { // Zelda Palette issue.
		int i = 0;
		//isFrozen = true;
		//return 5;
	}
	if (current_pc == 0x8030) {
		int i = 0;
	}
	if (current_pc == 0x30) {
		int i = 0;
	}
	if (isFrozen) {
		return 5;
	}

	// Mario 3 toad house scroll
	if (m_pc == 0xF795) {
		int scanline = bus->ppu.renderer->m_scanline;
		int i = 0;
	}

	//(this->*_opcodeTable[op])();
	// Check for interrupts before fetching next instruction
	// This must be placed AFTER the clock above. The reason is vblank related.
	// Some games, namely Dragon Warrior 3, wait for vblank in a dumb way. It loops on 2002 high bit while
	// interrupts are enabled and gets into a race condition.
	// If we check for interrupts before the clock, the interrupt will be serviced
	// immediately, which is incorrect behavior. The CPU should only check for interrupts after the current
	// instruction has fully executed.

	// NOTE TO NES developers: When interrupts are enabled, you should avoid reading $2002 in a tight loop like that.
	// You might miss the NMI entirely if it happens between your read and the interrupt check!
	// Have NMI set a variable that your loop waits for. Thank you!
	int cycles = checkInterrupts();
	cyclesThisFrame += cycles;
	uint8_t cyclesPassed =  m_cycle_count - cyclesBefore;
	cyclesThisFrame += cyclesPassed;
	return cyclesPassed;
}

uint8_t CPU::GetSP()
{
	return m_sp;
}

void CPU::SetSP(uint8_t sp)
{
	m_sp = sp;
}

inline uint8_t CPU::ReadNextByte()
{
	//dbg(L"0x%02X ", bus->read(m_pc));
	return bus->read(m_pc++);
}

inline uint8_t CPU::ReadNextByte(uint8_t offset)
{
	uint8_t base = ReadNextByte();
	uint8_t byte = (base + offset) & 0xFF; // Wraparound
	return byte;
}

inline uint16_t CPU::ReadNextWord()
{
	uint8_t loByte = ReadNextByte();
	uint8_t hiByte = ReadNextByte();
	return (static_cast<uint16_t>(hiByte << 8) | loByte);
}

inline uint16_t CPU::ReadNextWord(uint8_t offset)
{
	uint8_t loByte = ReadNextByte();
	uint8_t hiByte = ReadNextByte();
	loByte += offset;
	// Carryover?
	if (loByte < offset)
	{
		hiByte += 1;
		// One more cycle if a page is crossed.
		m_cycle_count++;
	}
	return (static_cast<uint16_t>(hiByte << 8) | loByte);
}

inline uint16_t CPU::ReadNextWordNoCycle(uint8_t offset)
{
	uint8_t loByte = ReadNextByte();
	uint8_t hiByte = ReadNextByte();
	loByte += offset;
	// Carryover?
	if (loByte < offset)
	{
		hiByte += 1;
	}
	return (static_cast<uint16_t>(hiByte << 8) | loByte);
}

inline uint16_t CPU::ReadIndexedIndirect()
{
	uint8_t zp_base = ReadNextByte();
	// Add X register to base (with zp wraparound)
	uint8_t zp_addr = (zp_base + m_x) & 0xFF;
	uint8_t addr_lo = bus->read(zp_addr);
	uint8_t addr_hi = bus->read((zp_addr + 1) & 0xFF); // Wraparound for the high byte
	return (addr_hi << 8) | addr_lo;
}

inline uint16_t CPU::ReadIndirectIndexed()
{
	uint8_t zp_base = ReadNextByte();
	uint8_t addr_lo = bus->read(zp_base);
	uint8_t addr_hi = bus->read((zp_base + 1) & 0xFF); // Wraparound for the high byte
	// Add Y register to the low byte of the address
	addr_lo += m_y;
	if (addr_lo < m_y) {
		addr_hi += 1; // Carryover
		m_cycle_count++; // Extra cycle for page crossing
	}
	return (addr_hi << 8) | addr_lo;
}

inline uint16_t CPU::ReadIndirectIndexedNoCycle()
{
	uint8_t zp_base = ReadNextByte();
	uint8_t addr_lo = bus->read(zp_base);
	uint8_t addr_hi = bus->read((zp_base + 1) & 0xFF); // Wraparound for the high byte
	// Add Y register to the low byte of the address
	addr_lo += m_y;
	if (addr_lo < m_y) {
		addr_hi += 1; // Carryover
	}
	return (addr_hi << 8) | addr_lo;
}

inline uint8_t CPU::ReadByte(uint16_t addr) {
	return bus->read(addr);
}

void CPU::WriteByte(uint16_t addr, uint8_t value) {
	bus->write(addr, value);
}

uint8_t CPU::GetStatus()
{
	return m_p;
}

void CPU::SetStatus(uint8_t status)
{
	m_p = status;
}

void CPU::ClearFlag(uint8_t flag)
{
	m_p &= ~flag;
}

inline void CPU::SetZero(uint8_t value)
{
	// Set/clear zero flag
	if (value == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
}

inline void CPU::SetNegative(uint8_t value)
{
	// Set/clear negative flag (bit 7 of result)
	if (value & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

inline void CPU::SetOverflow(bool condition)
{
	if (condition) {
		m_p |= FLAG_OVERFLOW;
	}
	else {
		m_p &= ~FLAG_OVERFLOW;
	}
}

inline void CPU::SetCarry(bool condition)
{
	if (condition) {
		m_p |= FLAG_CARRY;
	}
	else {
		m_p &= ~FLAG_CARRY;
	}
}

inline void CPU::SetDecimal(bool condition)
{
	if (condition) {
		m_p |= FLAG_DECIMAL;
	}
	else {
		m_p &= ~FLAG_DECIMAL;
	}
}

inline void CPU::SetInterrupt(bool condition)
{
	if (condition) {
		m_p |= FLAG_INTERRUPT;
	}
	else {
		m_p &= ~FLAG_INTERRUPT;
	}
}

inline void CPU::SetBreak(bool condition)
{
	if (condition) {
		m_p |= FLAG_BREAK;
	}
	else {
		m_p &= ~FLAG_BREAK;
	}
}

// Primarily used for testing purposes.
uint8_t CPU::GetA()
{
	return m_a;
}

void CPU::SetA(uint8_t a)
{
	m_a = a;
}

uint8_t CPU::GetX()
{
	return m_x;
}

void CPU::SetX(uint8_t x)
{
	m_x = x;
}

uint8_t CPU::GetY()
{
	return m_y;
}

void CPU::SetY(uint8_t y)
{
	m_y = y;
}

bool CPU::GetFlag(uint8_t flag)
{
	return (m_p & flag) == flag;
}

void CPU::SetFlag(uint8_t flag)
{
	m_p |= flag;
}

uint16_t CPU::GetPC() {
	return m_pc;
}

void CPU::SetPC(uint16_t address)
{
	m_pc = address;
}

void CPU::ConsumeCycle() {
	m_cycle_count++;
}

void CPU::buildMap() {
	instructionMap[0x69] = "ADC_IMMEDIATE";
	instructionMap[0x65] = "ADC_ZEROPAGE";
	instructionMap[0x75] = "ADC_ZEROPAGE_X";
	instructionMap[0x6D] = "ADC_ABSOLUTE";
	instructionMap[0x7D] = "ADC_ABSOLUTE_X";
	instructionMap[0x79] = "ADC_ABSOLUTE_Y";
	instructionMap[0x61] = "ADC_INDEXEDINDIRECT";
	instructionMap[0x71] = "ADC_INDIRECTINDEXED";
	instructionMap[0x29] = "AND_IMMEDIATE";
	instructionMap[0x25] = "AND_ZEROPAGE";
	instructionMap[0x35] = "AND_ZEROPAGE_X";
	instructionMap[0x2D] = "AND_ABSOLUTE";
	instructionMap[0x3D] = "AND_ABSOLUTE_X";
	instructionMap[0x39] = "AND_ABSOLUTE_Y";
	instructionMap[0x21] = "AND_INDEXEDINDIRECT";
	instructionMap[0x31] = "AND_INDIRECTINDEXED";
	instructionMap[0x0A] = "ASL_ACCUMULATOR";
	instructionMap[0x06] = "ASL_ZEROPAGE";
	instructionMap[0x16] = "ASL_ZEROPAGE_X";
	instructionMap[0x0E] = "ASL_ABSOLUTE";
	instructionMap[0x1E] = "ASL_ABSOLUTE_X";
	instructionMap[0x90] = "BCC_RELATIVE";
	instructionMap[0xB0] = "BCS_RELATIVE";
	instructionMap[0xF0] = "BEQ_RELATIVE";
	instructionMap[0x24] = "BIT_ZEROPAGE";
	instructionMap[0x2C] = "BIT_ABSOLUTE";
	instructionMap[0x30] = "BMI_RELATIVE";
	instructionMap[0xD0] = "BNE_RELATIVE";
	instructionMap[0x10] = "BPL_RELATIVE";
	instructionMap[0x00] = "BRK_IMPLIED";
	instructionMap[0x50] = "BVC_RELATIVE";
	instructionMap[0x70] = "BVS_RELATIVE";
	instructionMap[0x18] = "CLC_IMPLIED";
	instructionMap[0xD8] = "CLD_IMPLIED";
	instructionMap[0x58] = "CLI_IMPLIED";
	instructionMap[0xB8] = "CLV_IMPLIED";
	instructionMap[0xC9] = "CMP_IMMEDIATE";
	instructionMap[0xC5] = "CMP_ZEROPAGE";
	instructionMap[0xD5] = "CMP_ZEROPAGE_X";
	instructionMap[0xCD] = "CMP_ABSOLUTE";
	instructionMap[0xDD] = "CMP_ABSOLUTE_X";
	instructionMap[0xD9] = "CMP_ABSOLUTE_Y";
	instructionMap[0x69] = "ADC_IMMEDIATE";
	instructionMap[0xC1] = "CMP_INDEXEDINDIRECT";
	instructionMap[0xD1] = "CMP_INDIRECTINDEXED";
	instructionMap[0xE0] = "CPX_IMMEDIATE";
	instructionMap[0xE4] = "CPX_ZEROPAGE";
	instructionMap[0xEC] = "CPX_ABSOLUTE";
	instructionMap[0xC0] = "CPY_IMMEDIATE";
	instructionMap[0xC4] = "CPY_ZEROPAGE";
	instructionMap[0xCC] = "CPY_ABSOLUTE";
	instructionMap[0xC6] = "DEC_ZEROPAGE";
	instructionMap[0xD6] = "DEC_ZEROPAGE_X";
	instructionMap[0xCE] = "DEC_ABSOLUTE";
	instructionMap[0xDE] = "DEC_ABSOLUTE_X";
	instructionMap[0xCA] = "DEX_IMPLIED";
	instructionMap[0x88] = "DEY_IMPLIED";
	instructionMap[0x49] = "EOR_IMMEDIATE";
	instructionMap[0x45] = "EOR_ZEROPAGE";
	instructionMap[0x55] = "EOR_ZEROPAGE_X";
	instructionMap[0x4D] = "EOR_ABSOLUTE";
	instructionMap[0x5D] = "EOR_ABSOLUTE_X";
	instructionMap[0x59] = "EOR_ABSOLUTE_Y";
	instructionMap[0x41] = "EOR_INDEXEDINDIRECT";
	instructionMap[0x51] = "EOR_INDIRECTINDEXED";
	instructionMap[0xE6] = "INC_ZEROPAGE";
	instructionMap[0xF6] = "INC_ZEROPAGE_X";
	instructionMap[0xEE] = "INC_ABSOLUTE";
	instructionMap[0xFE] = "INC_ABSOLUTE_X";
	instructionMap[0xE8] = "INX_IMPLIED";
	instructionMap[0xC8] = "INY_IMPLIED";
	instructionMap[0x4C] = "JMP_ABSOLUTE";
	instructionMap[0x6C] = "JMP_INDIRECT";
	instructionMap[0x20] = "JSR_ABSOLUTE";
	instructionMap[0xA9] = "LDA_IMMEDIATE";
	instructionMap[0xA5] = "LDA_ZEROPAGE";
	instructionMap[0xB5] = "LDA_ZEROPAGE_X";
	instructionMap[0xAD] = "LDA_ABSOLUTE";
	instructionMap[0xBD] = "LDA_ABSOLUTE_X";
	instructionMap[0xB9] = "LDA_ABSOLUTE_Y";
	instructionMap[0xA1] = "LDA_INDEXEDINDIRECT";
	instructionMap[0xB1] = "LDA_INDIRECTINDEXED";
	instructionMap[0xA2] = "LDX_IMMEDIATE";
	instructionMap[0xA6] = "LDX_ZEROPAGE";
	instructionMap[0xB6] = "LDX_ZEROPAGE_Y";
	instructionMap[0xAE] = "LDX_ABSOLUTE";
	instructionMap[0xBE] = "LDX_ABSOLUTE_Y";
	instructionMap[0xA0] = "LDY_IMMEDIATE";
	instructionMap[0xA4] = "LDY_ZEROPAGE";
	instructionMap[0xB4] = "LDY_ZEROPAGE_X";
	instructionMap[0xAC] = "LDY_ABSOLUTE";
	instructionMap[0xBC] = "LDY_ABSOLUTE_X";
	instructionMap[0x4A] = "LSR_ACCUMULATOR";
	instructionMap[0x46] = "LSR_ZEROPAGE";
	instructionMap[0x56] = "LSR_ZEROPAGE_X";
	instructionMap[0x4E] = "LSR_ABSOLUTE";
	instructionMap[0x5E] = "LSR_ABSOLUTE_X";
	instructionMap[0xEA] = "NOP_IMPLIED";
	instructionMap[0x09] = "ORA_IMMEDIATE";
	instructionMap[0x05] = "ORA_ZEROPAGE";
	instructionMap[0x15] = "ORA_ZEROPAGE_X";
	instructionMap[0x0D] = "ORA_ABSOLUTE";
	instructionMap[0x1D] = "ORA_ABSOLUTE_X";
	instructionMap[0x19] = "ORA_ABSOLUTE_Y";
	instructionMap[0x01] = "ORA_INDEXEDINDIRECT";
	instructionMap[0x11] = "ORA_INDIRECTINDEXED";
	instructionMap[0x48] = "PHA_IMPLIED";
	instructionMap[0x08] = "PHP_IMPLIED";
	instructionMap[0x68] = "PLA_IMPLIED";
	instructionMap[0x28] = "PLP_IMPLIED";
	instructionMap[0x2A] = "ROL_ACCUMULATOR";
	instructionMap[0x26] = "ROL_ZEROPAGE";
	instructionMap[0x36] = "ROL_ZEROPAGE_X";
	instructionMap[0x2E] = "ROL_ABSOLUTE";
	instructionMap[0x3E] = "ROL_ABSOLUTE_X";
	instructionMap[0x6A] = "ROR_ACCUMULATOR";
	instructionMap[0x66] = "ROR_ZEROPAGE";
	instructionMap[0x76] = "ROR_ZEROPAGE_X";
	instructionMap[0x6E] = "ROR_ABSOLUTE";
	instructionMap[0x7E] = "ROR_ABSOLUTE_X";
	instructionMap[0x40] = "RTI_IMPLIED";
	instructionMap[0x60] = "RTS_IMPLIED";
	instructionMap[0xE9] = "SBC_IMMEDIATE";
	instructionMap[0xE5] = "SBC_ZEROPAGE";
	instructionMap[0xF5] = "SBC_ZEROPAGE_X";
	instructionMap[0xED] = "SBC_ABSOLUTE";
	instructionMap[0xFD] = "SBC_ABSOLUTE_X";
	instructionMap[0xF9] = "SBC_ABSOLUTE_Y";
	instructionMap[0xE1] = "SBC_INDEXEDINDIRECT";
	instructionMap[0xF1] = "SBC_INDIRECTINDEXED";
	instructionMap[0x38] = "SEC_IMPLIED";
	instructionMap[0xF8] = "SED_IMPLIED";
	instructionMap[0x78] = "SEI_IMPLIED";
	instructionMap[0x85] = "STA_ZEROPAGE";
	instructionMap[0x95] = "STA_ZEROPAGE_X";
	instructionMap[0x8D] = "STA_ABSOLUTE";
	instructionMap[0x9D] = "STA_ABSOLUTE_X";
	instructionMap[0x99] = "STA_ABSOLUTE_Y";
	instructionMap[0x81] = "STA_INDEXEDINDIRECT";
	instructionMap[0x91] = "STA_INDIRECTINDEXED";
	instructionMap[0x86] = "STX_ZEROPAGE";
	instructionMap[0x96] = "STX_ZEROPAGE_Y";
	instructionMap[0x8E] = "STX_ABSOLUTE";
	instructionMap[0x84] = "STY_ZEROPAGE";
	instructionMap[0x94] = "STY_ZEROPAGE_X";
	instructionMap[0x8C] = "STY_ABSOLUTE";
	instructionMap[0xAA] = "TAX_IMPLIED";
	instructionMap[0xA8] = "TAY_IMPLIED";
	instructionMap[0xBA] = "TSX_IMPLIED";
	instructionMap[0x8A] = "TXA_IMPLIED";
	instructionMap[0x9A] = "TXS_IMPLIED";
	instructionMap[0x98] = "TYA_IMPLIED";
}