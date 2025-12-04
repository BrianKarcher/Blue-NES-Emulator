#include "CPU.h"
#include "Bus.h"
#include <Windows.h>
#include "PPU.h"
#include "RendererLoopy.h"

/* We emulate the 6502 only as far as it is compatible with the NES. For example, we do not include Decimal Mode.*/

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html
int cnt = 0;
int cnt2 = 0;

// ---------------- Debug helper ----------------
inline void Processor_6502::dbg(const wchar_t* fmt, ...) {
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

inline void Processor_6502::dbgNmi(const wchar_t* fmt, ...) {
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

Processor_6502::Processor_6502() {
	buildMap();
}

void Processor_6502::PowerOn()
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
	nmiRequested = false;
	nmi_line = false;
	nmi_previous = false;
	irq_line = false;
	nmi_pending = false;
	isFrozen = false;
}

void Processor_6502::Activate(bool active)
{
	isActive = active;
}

void Processor_6502::Reset()
{
	// TODO : Add the reset vector read from the bus
	m_pc = (static_cast<uint16_t>(bus->read(0xFFFD) << 8)) | bus->read(0xFFFC);
	m_a = 0;
	m_x = 0;
	m_y = 0;
	m_p = 0x24; // Set unused flag bit to 1, others to 0
	m_sp = 0xFD;
	m_cycle_count = 0;
	nmiRequested = false;
	nmi_line = false;
	nmi_previous = false;
	irq_line = false;
	nmi_pending = false;
	isFrozen = false;
}

void Processor_6502::AddCycles(int count)
{
	m_cycle_count += count;
}

uint64_t Processor_6502::GetCycleCount()
{
	return m_cycle_count;
}

void Processor_6502::handleNMI() {
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
void Processor_6502::setNMI(bool state) {
	nmi_line = state;
}

void Processor_6502::setIRQ(bool state) {
	irq_line = state;
}

void Processor_6502::push(uint8_t value) {
	bus->write(0x0100 + m_sp, value);
	m_sp--;
}

uint8_t Processor_6502::pull() {
	m_sp++;
	return bus->read(0x0100 + m_sp);
}

void Processor_6502::handleIRQ() {
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

void Processor_6502::checkInterrupts() {
	// NMI - Edge-triggered (detects 0->1 transition)
	if (nmi_line && !nmi_previous) {
		nmi_pending = true;
	}
	nmi_previous = nmi_line;

	// Handle NMI (higher priority than IRQ)
	if (nmi_pending) {
		handleNMI();
		nmi_pending = false;
		return;  // Don't check IRQ if NMI occurred
	}

	// IRQ - Level-triggered, maskable by I flag
	// Check if IRQ line is active AND hardware interrupts are enabled
	if (irq_line && !(m_p & FLAG_INTERRUPT)) {
	//if (irq_line) {
		handleIRQ();
		irq_line = false; // Clear IRQ line after handling
	}
}

/// <summary>
/// Speed is paramount so we try to reduce function hops as much as feasible while keeping the code readable.
/// </summary>
uint8_t Processor_6502::Clock()
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
		int scanline = bus->ppu->renderer->m_scanline;
		int i = 0;
	}

	switch (op)
	{
		case ADC_IMMEDIATE:
		{
			// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
			ADC(ReadNextByte());
			m_cycle_count += 2;
			break;
		}
		case ADC_ZEROPAGE:
		{
			// An instruction using zero page addressing mode has only an 8 bit address operand.
			// This limits it to addressing only the first 256 bytes of memory (e.g. $0000 to $00FF)
			// where the most significant byte of the address is always zero.
			uint8_t zp_addr = ReadNextByte();
			ADC(ReadByte(zp_addr));
			m_cycle_count += 3;
			break;
		}
		case ADC_ZEROPAGE_X:
		{
			// The address to be accessed by an instruction using indexed zero page
			// addressing is calculated by taking the 8 bit zero page address from the
			// instruction and adding the current value of the X register to it.
			// The address calculation wraps around if the sum of the base address and the register exceed $FF.
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t offset = ReadByte(zp_addr);
			ADC(offset);
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE:
		{
			uint16_t memoryLocation = ReadNextWord();
			ADC(bus->read(memoryLocation));
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE_X:
		{
			// I'm doing the addition in 8-bit mostly because it makes the page crossing easier
			// to check to increment the cycle count.
			// I THINK this is less cycles than if I recorded the bytes in a 16-bit value.
			// This could all be sped up if I programmed it in Assembly but I don't want my code to be
			// CPU specific.
			uint16_t memoryLocation = ReadNextWord(m_x);
			ADC(bus->read(memoryLocation));
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE_Y:
		{
			uint16_t memoryLocation = ReadNextWord(m_y);
			ADC(ReadByte(memoryLocation));
			m_cycle_count += 4;
			break;
		}
		case ADC_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			ADC(operand);
			m_cycle_count += 6;
			break;
		}
		case ADC_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			ADC(operand);
			m_cycle_count += 5;
			break;
		}
		case AND_IMMEDIATE:
		{
			// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
			uint8_t operand = ReadNextByte();
			_and(operand);
			m_cycle_count += 2;
			break;
		}
		case AND_ZEROPAGE:
		{
			// An instruction using zero page addressing mode has only an 8 bit address operand.
			// This limits it to addressing only the first 256 bytes of memory (e.g. $0000 to $00FF)
			// where the most significant byte of the address is always zero.
			uint8_t operand = ReadByte(ReadNextByte());
			_and(operand);
			m_cycle_count += 3;
			break;
		}
		case AND_ZEROPAGE_X:
		{
			// The address to be accessed by an instruction using indexed zero page
			// addressing is calculated by taking the 8 bit zero page address from the
			// instruction and adding the current value of the X register to it.
			// The address calculation wraps around if the sum of the base address and the register exceed $FF.
			uint8_t operand = ReadByte(ReadNextByte(m_x));
			_and(operand);
			m_cycle_count += 4;
			break;
		}
		case AND_ABSOLUTE:
		{
			uint16_t memoryLocation = ReadNextWord();
			uint8_t operand = ReadByte(memoryLocation);
			_and(operand);
			m_cycle_count += 4;
			break;
		}
		case AND_ABSOLUTE_X:
		{
			uint16_t memoryLocation = ReadNextWord(m_x);
			uint8_t operand = ReadByte(memoryLocation);
			_and(operand);
			m_cycle_count += 4;
			break;
		}
		case AND_ABSOLUTE_Y:
		{
			uint16_t memoryLocation = ReadNextWord(m_y);
			uint8_t operand = ReadByte(memoryLocation);
			_and(operand);
			m_cycle_count += 4;
			break;
		}
		case AND_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			_and(operand);
			m_cycle_count += 6;
			break;
		}
		case AND_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			_and(operand);
			m_cycle_count += 5;
			break;
		}
		case ASL_ACCUMULATOR:
		{
			ASL(m_a);
			m_cycle_count += 2;
			break;
		}
		case ASL_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			ASL(data);
			bus->write(zp_addr, data);
			m_cycle_count += 5;
			break;
		}
		case ASL_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			ASL(data);
			bus->write(zp_addr, data);
			m_cycle_count += 6;
			break;
		}
		case ASL_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t addr = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t data = ReadByte(addr);
			ASL(data);
			bus->write(addr, data);
			m_cycle_count += 6;
			break;
		}
		case ASL_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t data = ReadByte(addr);
			ASL(data);
			bus->write(addr, data);
			m_cycle_count += 7;
			break;
		}
		case BCC_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			if (!(m_p & FLAG_CARRY)) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BCS_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			if (m_p & FLAG_CARRY) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BEQ_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			if (m_p & FLAG_ZERO) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BIT_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			BIT(data);
			break;
		}
		case BIT_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t addr = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t data = ReadByte(addr);
			BIT(data);
			break;
		}
		case BMI_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			if (m_p & FLAG_NEGATIVE) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BNE_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			dbg(L"0x%02X", offset);
			if (!(m_p & FLAG_ZERO)) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BPL_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			dbg(L"0x%02X", offset);
			if (!(m_p & FLAG_NEGATIVE)) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BRK_IMPLIED:
		{
			//m_p |= FLAG_BREAK; // Set break flag
			uint8_t flags = m_p | FLAG_BREAK | FLAG_UNUSED;
			uint16_t return_addr = static_cast<uint16_t>(m_pc + 1); // Increment PC by 1 to skip over the next byte (padding byte)
			//m_pc += 1; 
			// Push PC and P to stack
			bus->write(0x0100 + m_sp--, (return_addr >> 8) & 0xFF); // Push high byte of PC
			bus->write(0x0100 + m_sp--, return_addr & 0xFF);        // Push low byte of PC
			bus->write(0x0100 + m_sp--, flags);                // Push processor status
			// Set PC to NMI vector
			m_pc = (static_cast<uint16_t>(ReadByte(0xFFFF) << 8)) | ReadByte(0xFFFE);
			// Set Interrupt Disable flag in real status so IRQs are masked
			m_p |= FLAG_INTERRUPT;
			m_cycle_count += 7;
			break;
		}
		case BVC_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			dbg(L"0x%02X", offset);
			if (!(m_p & FLAG_OVERFLOW)) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case BVS_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
			dbg(L"0x%02X", offset);
			if (m_p & FLAG_OVERFLOW) {
				if (NearBranch(offset))
					m_cycle_count++; // Extra cycle for page crossing
				m_cycle_count += 3; // Branch taken
			}
			else {
				m_cycle_count += 2; // Branch not taken
			}
			break;
		}
		case CLC_IMPLIED:
		{
			m_p &= ~FLAG_CARRY; // Clear carry flag
			m_cycle_count += 2;
			break;
		}
		case CLD_IMPLIED:
		{
			m_p &= ~FLAG_DECIMAL; // Clear decimal mode flag
			m_cycle_count += 2;
			break;
		}
		case CLI_IMPLIED:
		{
			m_p &= ~FLAG_INTERRUPT; // Clear interrupt disable flag
			m_cycle_count += 2;
			break;
		}
		case CLV_IMPLIED:
		{
			m_p &= ~FLAG_OVERFLOW; // Clear overflow flag
			m_cycle_count += 2;
			break;
		}
		case CMP_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			cp(m_a, operand);
			m_cycle_count += 2;
			break;
		}
		case CMP_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			cp(m_a, operand);
			m_cycle_count += 3;
			break;
		}
		case CMP_ZEROPAGE_X:
		{
			uint8_t operand = ReadByte(ReadNextByte(m_x));
			cp(m_a, operand);
			m_cycle_count += 4;
			break;
		}
		case CMP_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t operand = ReadByte(memoryLocation);
			cp(m_a, operand);
			m_cycle_count += 4;
			break;
		}
		case CMP_ABSOLUTE_X:
		{
			uint16_t memoryLocation = ReadNextWord(m_x);
			uint8_t operand = ReadByte(memoryLocation);
			cp(m_a, operand);
			m_cycle_count += 4;
			break;
		}
		case CMP_ABSOLUTE_Y:
		{
			uint16_t memoryLocation = ReadNextWord(m_y);
			uint8_t operand = ReadByte(memoryLocation);
			cp(m_a, operand);
			m_cycle_count += 4;
			break;
		}
		case CMP_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			cp(m_a, operand);
			m_cycle_count += 6;
			break;
		}
		case CMP_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			cp(m_a, operand);
			m_cycle_count += 5;
			break;
		}
		case CPX_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			cp(m_x, operand);
			m_cycle_count += 2;
			break;
		}
		case CPX_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			cp(m_x, operand);
			m_cycle_count += 3;
			break;
		}
		case CPX_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t operand = ReadByte(memoryLocation);
			cp(m_x, operand);
			m_cycle_count += 4;
			break;
		}
		case CPY_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			cp(m_y, operand);
			m_cycle_count += 2;
			break;
		}
		case CPY_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			cp(m_y, operand);
			m_cycle_count += 3;
			break;
		}
		case CPY_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t operand = ReadByte(memoryLocation);
			cp(m_y, operand);
			m_cycle_count += 4;
			break;
		}
		case DEC_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			data--;
			bus->write(zp_addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 5;
			break;
		}
		case DEC_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			data--;
			bus->write(zp_addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 6;
			break;
		}
		case DEC_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t addr = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t data = ReadByte(addr);
			data--;
			bus->write(addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 6;
			break;
		}
		case DEC_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t data = ReadByte(addr);
			data--;
			bus->write(addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 7;
			break;
		}
		case DEX_IMPLIED:
		{
			m_x--;
			// Set/clear zero flag
			SetZero(m_x);
			// Set/clear negative flag (bit 7 of result)
			SetNegative(m_x);
			m_cycle_count += 2;
			break;
		}
		case DEY_IMPLIED:
		{
			m_y--;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 2;
			break;
		}
		case EOR_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			EOR(operand);
			m_cycle_count += 2;
			break;
		}
		case EOR_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			EOR(operand);
			m_cycle_count += 3;
			break;
		}
		case EOR_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t operand = ReadByte(zp_addr);
			EOR(operand);
			m_cycle_count += 4;
			break;
		}
		case EOR_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t addr = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t operand = ReadByte(addr);
			EOR(operand);
			m_cycle_count += 4;
			break;
		}
		case EOR_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t operand = ReadByte(addr);
			EOR(operand);
			m_cycle_count += 4;
			break;
		}
		case EOR_ABSOLUTE_Y:
		{
			uint16_t addr = ReadNextWord(m_y);
			uint8_t operand = ReadByte(addr);
			EOR(operand);
			m_cycle_count += 4;
			break;
		}
		case EOR_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			EOR(operand);
			m_cycle_count += 6;
			break;
		}
		case EOR_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			EOR(operand);
			m_cycle_count += 5;
			break;
		}
		case INC_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			data++;
			bus->write(zp_addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 5;
			break;
		}
		case INC_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			data++;
			bus->write(zp_addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 6;
			break;
		}
		case INC_ABSOLUTE:
		{
			uint8_t loByte = ReadNextByte();
			uint8_t hiByte = ReadNextByte();
			uint16_t addr = (static_cast<uint16_t>(hiByte << 8) | loByte);
			uint8_t data = ReadByte(addr);
			dbg(L"0x%04X = 0x%02X", addr, data);
			data++;
			bus->write(addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 6;
			break;
		}
		case INC_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t data = ReadByte(addr);
			data++;
			bus->write(addr, data);
			SetZero(data);
			SetNegative(data);
			m_cycle_count += 7;
			break;
		}
		case INX_IMPLIED:
		{
			m_x++;
			// Set/clear zero flag
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 2;
			break;
		}
		case INY_IMPLIED:
		{
			m_y++;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 2;
			break;
		}
		case JMP_ABSOLUTE:
		{
			m_pc = ReadNextWord();
			dbg(L"0x%04X", m_pc);
			m_cycle_count += 3;
			break;
		}
		case JMP_INDIRECT:
		{
			uint16_t pointer_addr = ReadNextWord();
			// Simulate page boundary hardware bug
			uint8_t loByte = ReadByte(pointer_addr);
			uint8_t hiByte;
			if ((pointer_addr & 0x00FF) == 0x00FF) {
				hiByte = ReadByte(pointer_addr & 0xFF00); // Wraparound
			}
			else {
				hiByte = ReadByte(pointer_addr + 1);
			}
			dbg(L"0x%04X 0x%02X 0x%02X", pointer_addr, loByte, hiByte);
			m_pc = (static_cast<uint16_t>(hiByte << 8) | loByte);
			m_cycle_count += 5;
			break;
		}
		case JSR_ABSOLUTE:
		{
			uint16_t return_addr = m_pc + 1; // Address to return to after subroutine
			// Push return address onto stack (high byte first)
			bus->write(0x0100 + m_sp--, (return_addr >> 8) & 0xFF); // High byte
			dbg(L"Pushing 0x%02X to stack 0x%02X\n", (return_addr >> 8) & 0xFF, m_sp + 1);
			bus->write(0x0100 + m_sp--, return_addr & 0xFF);        // Low byte
			dbg(L"Pushing 0x%02X to stack 0x%02X\n", return_addr & 0xFF, m_sp + 1);
			// Set PC to target address
			m_pc = ReadNextWord();
			dbg(L"JSR to 0x%04X (return 0x%04X)", m_pc, return_addr);
			m_cycle_count += 6;
			break;
		}
		case LDA_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			dbg(L"0x%02X", operand);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 2;
			break;
		}
		case LDA_ZEROPAGE:
		{
			uint16_t addr = ReadNextByte();
			uint8_t operand = ReadByte(addr);
			dbg(L"0x%04X 0x%02X", addr, operand);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 3;
			break;
		}
		case LDA_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t operand = ReadByte(zp_addr);
			dbg(L"0x%02X + 0x%02X = 0x%02X", zp_addr, m_x, operand);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 4;
			break;
		}
		case LDA_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t operand = ReadByte(addr);
			dbg(L"0x%04X = 0x%02X", addr, operand);
			if (addr == 0x2002) {
				//dbg(L"LDA 2002, operand=%d", operand);
			}
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 4;
			break;
		}
		case LDA_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t operand = ReadByte(addr);
			dbg(L"0x%04X = 0x%02X for X 0x%02X", addr, operand, m_x);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 4;
			break;
		}
		case LDA_ABSOLUTE_Y:
		{
			uint16_t addr = ReadNextWord(m_y);
			uint8_t operand = ReadByte(addr);
			dbg(L"0x%04X = 0x%02X for Y 0x%02X", addr, operand, m_y);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 4;
			break;
		}
		case LDA_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			dbg(L"Target addr 0x%04X = 0x%02X", target_addr, operand);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 6;
			break;
		}
		case LDA_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 5;
			break;
		}
		case LDX_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			m_x = operand;
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 2;
			break;
		}
		case LDX_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			m_x = operand;
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 3;
			break;
		}
		case LDX_ZEROPAGE_Y:
		{
			uint8_t zp_addr = ReadNextByte(m_y);
			uint8_t operand = ReadByte(zp_addr);
			m_x = operand;
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 4;
			break;
		}
		case LDX_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t operand = ReadByte(addr);
			m_x = operand;
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 4;
			break;
		}
		case LDX_ABSOLUTE_Y:
		{
			uint16_t addr = ReadNextWord(m_y);
			uint8_t operand = ReadByte(addr);
			m_x = operand;
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 4;
			break;
		}
		case LDY_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			m_y = operand;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 2;
			break;
		}
		case LDY_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			m_y = operand;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 3;
			break;
		}
		case LDY_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t operand = ReadByte(zp_addr);
			m_y = operand;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 4;
			break;
		}
		case LDY_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t operand = ReadByte(addr);
			m_y = operand;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 4;
			break;
		}
		case LDY_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t operand = ReadByte(addr);
			m_y = operand;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 4;
			break;
		}
		case LSR_ACCUMULATOR:
		{
			LSR(m_a);
			m_cycle_count += 2;
			break;
		}
		case LSR_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			LSR(data);
			bus->write(zp_addr, data);
			m_cycle_count += 5;
			break;
		}
		case LSR_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			LSR(data);
			bus->write(zp_addr, data);
			m_cycle_count += 6;
			break;
		}
		case LSR_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t data = ReadByte(addr);
			LSR(data);
			bus->write(addr, data);
			m_cycle_count += 6;
			break;
		}
		case LSR_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t data = ReadByte(addr);
			LSR(data);
			bus->write(addr, data);
			m_cycle_count += 7;
			break;
		}
		case NOP_IMPLIED:
		{
			// No operation
			m_cycle_count += 2;
			break;
		}
		case ORA_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			ORA(operand);
			m_cycle_count += 2;
			break;
		}
		case ORA_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			ORA(data);
			m_cycle_count += 3;
			break;
		}
		case ORA_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			ORA(data);
			m_cycle_count += 4;
			break;
		}
		case ORA_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t data = ReadByte(addr);
			ORA(data);
			m_cycle_count += 4;
			break;
		}
		case ORA_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t data = ReadByte(addr);
			ORA(data);
			m_cycle_count += 4;
			break;
		}
		case ORA_ABSOLUTE_Y:
		{
			uint16_t addr = ReadNextWord(m_y);
			uint8_t data = ReadByte(addr);
			ORA(data);
			m_cycle_count += 4;
			break;
		}
		case ORA_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			ORA(operand);
			m_cycle_count += 6;
			break;
		}
		case ORA_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			ORA(operand);
			m_cycle_count += 5;
			break;
		}
		case PHA_IMPLIED:
		{
			bus->write(0x0100 + m_sp--, m_a);
			m_cycle_count += 3;
			break;
		}
		case PHP_IMPLIED:
		{
			bus->write(0x0100 + m_sp--, m_p | FLAG_BREAK | 0x20); // Set B and unused flag bits when pushing P
			m_cycle_count += 3;
			break;
		}
		case PLA_IMPLIED:
		{
			m_a = ReadByte(0x0100 + ++m_sp);
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 4;
			break;
		}
		case PLP_IMPLIED:
		{
			bool break_flag = (m_p & FLAG_BREAK) != 0; // Save current B flag state
			m_p = ReadByte(0x0100 + ++m_sp);
			SetBreak(break_flag); // Restore B flag state
			m_cycle_count += 4;
			break;
		}
		case ROL_ACCUMULATOR:
		{
			ROL(m_a);
			m_cycle_count += 2;
			break;
		}
		case ROL_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			ROL(data);
			bus->write(zp_addr, data);
			m_cycle_count += 5;
			break;
		}
		case ROL_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			ROL(data);
			bus->write(zp_addr, data);
			m_cycle_count += 6;
			break;
		}
		case ROL_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t data = ReadByte(addr);
			ROL(data);
			bus->write(addr, data);
			m_cycle_count += 6;
			break;
		}
		case ROL_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWordNoCycle(m_x);
			uint8_t data = ReadByte(addr);
			ROL(data);
			bus->write(addr, data);
			m_cycle_count += 7;
			break;
		}
		case ROR_ACCUMULATOR:
		{
			ROR(m_a);
			m_cycle_count += 2;
			break;
		}
		case ROR_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			uint8_t data = ReadByte(zp_addr);
			ROR(data);
			bus->write(zp_addr, data);
			m_cycle_count += 5;
			break;
		}
		case ROR_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t data = ReadByte(zp_addr);
			ROR(data);
			bus->write(zp_addr, data);
			m_cycle_count += 6;
			break;
		}
		case ROR_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t data = ReadByte(addr);
			ROR(data);
			bus->write(addr, data);
			m_cycle_count += 6;
			break;
		}
		case ROR_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWordNoCycle(m_x);
			uint8_t data = ReadByte(addr);
			ROR(data);
			bus->write(addr, data);
			m_cycle_count += 7;
			break;
		}
		case RTI_IMPLIED:
		{
			//bool break_flag = (m_p & FLAG_BREAK) != 0; // Save current B flag state
			// Pull P from stack
			dbgNmi(L"\nRTI (cycle %d)\n", m_cycle_count);
			uint8_t pulledP = ReadByte(0x0100 + ++m_sp);
			dbg(L"Readomg 0x%02X from stack 0x%02X\n", pulledP, m_sp - 1);

			m_p = static_cast<uint8_t>((pulledP & ~FLAG_BREAK) | FLAG_UNUSED);

			//SetBreak(break_flag); // Restore B flag state
			//SetInterrupt(false);
			// Pull PC from stack (low byte first)
			uint8_t pc_lo = ReadByte(0x0100 + ++m_sp);
			dbg(L"Readomg 0x%02X from stack 0x%02X\n", pc_lo, m_sp - 1);
			uint8_t pc_hi = ReadByte(0x0100 + ++m_sp);
			dbg(L"Readomg 0x%02X from stack 0x%02X\n", pc_hi, m_sp - 1);
			m_pc = (static_cast<uint16_t>(pc_hi << 8) | pc_lo);
			dbg(L"Setting PC to 0x%04X\n", m_pc);
			m_cycle_count += 6;
			break;
		}
		case RTS_IMPLIED:
		{
			// Pull return address from stack (low byte first)
			uint8_t pc_lo = ReadByte(0x0100 + ++m_sp);
			dbg(L"Pulling 0x%02X from stack 0x%02X\n", pc_lo, m_sp);
			uint8_t pc_hi = ReadByte(0x0100 + ++m_sp);
			dbg(L"Pulling 0x%02X from stack 0x%02X\n", pc_hi, m_sp);
			m_pc = (static_cast<uint16_t>(pc_hi << 8) | pc_lo);
			m_pc++; // Increment PC to point to the next instruction after the JSR
			dbg(L"Changing pc to 0x%04X\n", m_pc);
			m_cycle_count += 6;
			break;
		}
		case SBC_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			SBC(operand);
			m_cycle_count += 2;
			break;
		}
		case SBC_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
			SBC(operand);
			m_cycle_count += 3;
			break;
		}
		case SBC_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			uint8_t operand = ReadByte(zp_addr);
			SBC(operand);
			m_cycle_count += 4;
			break;
		}
		case SBC_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			uint8_t operand = ReadByte(addr);
			SBC(operand);
			m_cycle_count += 4;
			break;
		}
		case SBC_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWord(m_x);
			uint8_t operand = ReadByte(addr);
			SBC(operand);
			m_cycle_count += 4;
			break;
		}
		case SBC_ABSOLUTE_Y:
		{
			uint16_t addr = ReadNextWord(m_y);
			uint8_t operand = ReadByte(addr);
			SBC(operand);
			m_cycle_count += 4;
			break;
		}
		case SBC_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			uint8_t operand = ReadByte(target_addr);
			SBC(operand);
			m_cycle_count += 6;
			break;
		}
		case SBC_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexed();
			uint8_t operand = ReadByte(target_addr);
			SBC(operand);
			m_cycle_count += 5;
			break;
		}
		case SEC_IMPLIED:
		{
			m_p |= FLAG_CARRY; // Set carry flag
			m_cycle_count += 2;
			break;
		}
		case SED_IMPLIED:
		{
			m_p |= FLAG_DECIMAL; // Set decimal mode flag
			m_cycle_count += 2;
			break;
		}
		case SEI_IMPLIED:
		{
			m_p |= FLAG_INTERRUPT; // Set interrupt disable flag
			m_cycle_count += 2;
			break;
		}
		case STA_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			bus->write(zp_addr, m_a);
			m_cycle_count += 3;
			break;
		}
		case STA_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			bus->write(zp_addr, m_a);
			m_cycle_count += 4;
			break;
		}
		case STA_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			dbg(L"0x%04X", addr);
			if (addr >= 0x2000 && addr <= 0x2FFF) {
				int i = 0;
			}
			if (addr == 0x2007) {
				if (cnt2 == 2000) {
					int j = 0;
				}
			}
			bus->write(addr, m_a);
			m_cycle_count += 4;
			break;
		}
		case STA_ABSOLUTE_X:
		{
			uint16_t addr = ReadNextWordNoCycle(m_x);
			if (addr >= 0x0200 && addr <= 0x02FF) {
				int i = 0;
			}
			bus->write(addr, m_a);
			m_cycle_count += 5;
			break;
		}
		case STA_ABSOLUTE_Y:
		{
			uint16_t addr = ReadNextWordNoCycle(m_y);
			if (addr >= 0x0200 && addr <= 0x02FF) {
				int i = 0;
			}
			bus->write(addr, m_a);
			m_cycle_count += 5;
			break;
		}
		case STA_INDEXEDINDIRECT:
		{
			uint16_t target_addr = ReadIndexedIndirect();
			bus->write(target_addr, m_a);
			m_cycle_count += 6;
			break;
		}
		case STA_INDIRECTINDEXED:
		{
			uint16_t target_addr = ReadIndirectIndexedNoCycle();
			bus->write(target_addr, m_a);
			m_cycle_count += 6;
			break;
		}
		case STX_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			bus->write(zp_addr, m_x);
			m_cycle_count += 3;
			break;
		}
		case STX_ZEROPAGE_Y:
		{
			uint8_t zp_addr = ReadNextByte(m_y);
			bus->write(zp_addr, m_x);
			m_cycle_count += 4;
			break;
		}
		case STX_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			bus->write(addr, m_x);
			m_cycle_count += 4;
			break;
		}
		case STY_ZEROPAGE:
		{
			uint8_t zp_addr = ReadNextByte();
			bus->write(zp_addr, m_y);
			m_cycle_count += 3;
			break;
		}
		case STY_ZEROPAGE_X:
		{
			uint8_t zp_addr = ReadNextByte(m_x);
			bus->write(zp_addr, m_y);
			m_cycle_count += 4;
			break;
		}
		case STY_ABSOLUTE:
		{
			uint16_t addr = ReadNextWord();
			bus->write(addr, m_y);
			m_cycle_count += 4;
			break;
		}
		case TAX_IMPLIED:
		{
			m_x = m_a;
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 2;
			break;
		}
		case TAY_IMPLIED:
		{
			m_y = m_a;
			SetZero(m_y);
			SetNegative(m_y);
			m_cycle_count += 2;
			break;
		}
		case TSX_IMPLIED:
		{
			m_x = m_sp;
			dbg(L"Transferring sp 0x%02X to x\n", m_sp);
			SetZero(m_x);
			SetNegative(m_x);
			m_cycle_count += 2;
			break;
		}
		case TXA_IMPLIED:
		{
			m_a = m_x;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 2;
			break;
		}
		case TXS_IMPLIED:
		{
			m_sp = m_x;
			m_cycle_count += 2;
			break;
		}
		case TYA_IMPLIED:
		{
			m_a = m_y;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 2;
			break;
		}
	}
	uint8_t cyclesPassed =  m_cycle_count - cyclesBefore;
	cyclesThisFrame += cyclesPassed;
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
	checkInterrupts();
	return cyclesPassed;
}

uint8_t Processor_6502::GetSP()
{
	return m_sp;
}

void Processor_6502::SetSP(uint8_t sp)
{
	m_sp = sp;
}

inline uint8_t Processor_6502::ReadNextByte()
{
	//dbg(L"0x%02X ", bus->read(m_pc));
	return bus->read(m_pc++);
}

inline uint8_t Processor_6502::ReadNextByte(uint8_t offset)
{
	uint8_t base = ReadNextByte();
	uint8_t byte = (base + offset) & 0xFF; // Wraparound
	return byte;
}

inline uint16_t Processor_6502::ReadNextWord()
{
	uint8_t loByte = ReadNextByte();
	uint8_t hiByte = ReadNextByte();
	return (static_cast<uint16_t>(hiByte << 8) | loByte);
}

inline uint16_t Processor_6502::ReadNextWord(uint8_t offset)
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

inline uint16_t Processor_6502::ReadNextWordNoCycle(uint8_t offset)
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

inline uint16_t Processor_6502::ReadIndexedIndirect()
{
	uint8_t zp_base = ReadNextByte();
	// Add X register to base (with zp wraparound)
	uint8_t zp_addr = (zp_base + m_x) & 0xFF;
	uint8_t addr_lo = bus->read(zp_addr);
	uint8_t addr_hi = bus->read((zp_addr + 1) & 0xFF); // Wraparound for the high byte
	return (addr_hi << 8) | addr_lo;
}

inline uint16_t Processor_6502::ReadIndirectIndexed()
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

inline uint16_t Processor_6502::ReadIndirectIndexedNoCycle()
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

inline uint8_t Processor_6502::ReadByte(uint16_t addr)
{
	return bus->read(addr);
}

uint8_t Processor_6502::GetStatus()
{
	return m_p;
}

void Processor_6502::SetStatus(uint8_t status)
{
	m_p = status;
}

void Processor_6502::ClearFlag(uint8_t flag)
{
	m_p &= ~flag;
}

inline void Processor_6502::SetZero(uint8_t value)
{
	// Set/clear zero flag
	if (value == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
}

inline void Processor_6502::SetNegative(uint8_t value)
{
	// Set/clear negative flag (bit 7 of result)
	if (value & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

inline void Processor_6502::SetOverflow(bool condition)
{
	if (condition) {
		m_p |= FLAG_OVERFLOW;
	}
	else {
		m_p &= ~FLAG_OVERFLOW;
	}
}

inline void Processor_6502::SetCarry(bool condition)
{
	if (condition) {
		m_p |= FLAG_CARRY;
	}
	else {
		m_p &= ~FLAG_CARRY;
	}
}

inline void Processor_6502::SetDecimal(bool condition)
{
	if (condition) {
		m_p |= FLAG_DECIMAL;
	}
	else {
		m_p &= ~FLAG_DECIMAL;
	}
}

inline void Processor_6502::SetInterrupt(bool condition)
{
	if (condition) {
		m_p |= FLAG_INTERRUPT;
	}
	else {
		m_p &= ~FLAG_INTERRUPT;
	}
}

inline void Processor_6502::SetBreak(bool condition)
{
	if (condition) {
		m_p |= FLAG_BREAK;
	}
	else {
		m_p &= ~FLAG_BREAK;
	}
}

void Processor_6502::ADC(uint8_t operand)
{
	uint8_t a_old = m_a;
	
	uint16_t result = m_a + operand + (m_p & FLAG_CARRY ? 1 : 0);
	m_a = result & 0xFF;  // Update accumulator with low byte
	// Set/clear carry flag
	if (result > 0xFF) {
		m_p |= FLAG_CARRY;   // Set carry
	}
	else {
		m_p &= ~FLAG_CARRY;  // Clear carry
	}
	// Set/clear overflow flag
	// Overflow occurs when:
	// - Adding two positive numbers results in a negative number, OR
	// - Adding two negative numbers results in a positive number
	// This can be detected by checking if the sign bits of both operands
	// are the same, but different from the result's sign bit
	if (((a_old ^ m_a) & (operand ^ m_a) & 0x80) != 0) {
		m_p |= FLAG_OVERFLOW;   // Set overflow
	}
	else {
		m_p &= ~FLAG_OVERFLOW;  // Clear overflow
	}
	// Set/clear zero flag
	if (m_a == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}

	// Set/clear negative flag (bit 7 of result)
	if (m_a & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

void Processor_6502::_and(uint8_t operand)
{
	// AND operation with the accumulator
	m_a &= operand;
	// Set/clear zero flag
	if (m_a == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)
	if (m_a & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

void Processor_6502::ASL(uint8_t& byte)
{
	// Set/clear carry flag based on bit 7
	if (byte & 0x80) {
		m_p |= FLAG_CARRY;   // Set carry
	}
	else {
		m_p &= ~FLAG_CARRY;  // Clear carry
	}
	// Shift left by 1
	byte <<= 1;
	// Set/clear zero flag
	if (byte == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)
	if (byte & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

bool Processor_6502::NearBranch(uint8_t value) {
	int8_t offset = static_cast<int8_t>(value); // Convert to signed
	// Determine if page crossed
	uint8_t old_pc_hi = (m_pc & 0xFF00) >> 8;
	m_pc += offset;
	uint8_t new_pc_hi = (m_pc & 0xFF00) >> 8;
	return old_pc_hi != new_pc_hi;
}

void Processor_6502::BIT(uint8_t data) {
	// Set zero flag based on AND with accumulator
	if ((m_a & data) == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set negative flag based on bit 7 of data
	if (data & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
	// Set overflow flag based on bit 6 of data
	if (data & 0x40) {
		m_p |= FLAG_OVERFLOW;
	}
	else {
		m_p &= ~FLAG_OVERFLOW;
	}
}

void Processor_6502::cp(uint8_t value, uint8_t operand)
{
	uint8_t result = value - operand;
	dbg(L"0x%02X", result);
	// Set/clear carry flag
	if (value >= operand) {
		m_p |= FLAG_CARRY;   // Set carry
	}
	else {
		m_p &= ~FLAG_CARRY;  // Clear carry
	}
	// Set/clear zero flag
	if (result == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)
	if (result & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

void Processor_6502::EOR(uint8_t operand)
{
	// EOR operation with the accumulator
	m_a ^= operand;
	dbg(L"0x%02X", m_a);
	// Set/clear zero flag
	if (m_a == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)
	if (m_a & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

void Processor_6502::LSR(uint8_t& byte)
{
	// Set/clear carry flag based on bit 0  
	if (byte & 0x01) {
		m_p |= FLAG_CARRY;   // Set carry  
	}
	else {
		m_p &= ~FLAG_CARRY;  // Clear carry  
	}
	// Shift right by 1
	byte >>= 1;
	dbg(L"0x%02X", byte);
	// Set/clear zero flag  
	if (byte == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)  
	m_p &= ~FLAG_NEGATIVE; // Clear negative flag since result of LSR is always positive  
}

void Processor_6502::ORA(uint8_t operand)
{
	// Perform bitwise OR operation with the accumulator  
	m_a |= operand;
	dbg(L"0x%02X", m_a);

	// Set/clear zero flag  
	if (m_a == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}

	// Set/clear negative flag (bit 7 of result)  
	if (m_a & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

void Processor_6502::ROL(uint8_t& byte)  
{  
	// Save the carry flag before modifying it  
	bool carry_in = (m_p & FLAG_CARRY) != 0;  

	// Set/clear carry flag based on bit 7  
	if (byte & 0x80) {  
		m_p |= FLAG_CARRY;   // Set carry  
	} else {  
		m_p &= ~FLAG_CARRY;  // Clear carry  
	}  

	// Rotate left by 1 (shift left and insert carry into bit 0)  
	byte = (byte << 1) | (carry_in ? 1 : 0);  

	// Set/clear zero flag  
	SetZero(byte);  

	// Set/clear negative flag (bit 7 of result)  
	SetNegative(byte);  
}

void Processor_6502::ROR(uint8_t& byte)
{
	// Save the carry flag before modifying it  
	bool carry_in = (m_p & FLAG_CARRY) != 0;  
	// Set/clear carry flag based on bit 0  
	if (byte & 0x01) {  
		m_p |= FLAG_CARRY;   // Set carry  
	} else {  
		m_p &= ~FLAG_CARRY;  // Clear carry  
	}  
	// Rotate right by 1 (shift right and insert carry into bit 7)  
	byte = (byte >> 1) | (carry_in ? 0x80 : 0);  
	// Set/clear zero flag  
	SetZero(byte);  
	// Set/clear negative flag (bit 7 of result)  
	SetNegative(byte);
}

void Processor_6502::SBC(uint8_t operand)
{
	// Invert the operand for subtraction
	uint8_t inverted_operand = ~operand;
	// Perform addition with inverted operand and carry flag
	ADC(inverted_operand);
}

// Primarily used for testing purposes.
uint8_t Processor_6502::GetA()
{
	return m_a;
}

void Processor_6502::SetA(uint8_t a)
{
	m_a = a;
}

uint8_t Processor_6502::GetX()
{
	return m_x;
}

void Processor_6502::SetX(uint8_t x)
{
	m_x = x;
}

uint8_t Processor_6502::GetY()
{
	return m_y;
}

void Processor_6502::SetY(uint8_t y)
{
	m_y = y;
}

bool Processor_6502::GetFlag(uint8_t flag)
{
	return (m_p & flag) == flag;
}

void Processor_6502::SetFlag(uint8_t flag)
{
	m_p |= flag;
}

uint16_t Processor_6502::GetPC() {
	return m_pc;
}

void Processor_6502::SetPC(uint16_t address)
{
	m_pc = address;
}

void Processor_6502::buildMap() {
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
