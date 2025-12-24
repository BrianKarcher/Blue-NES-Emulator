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

void CPU::BMI() {
	// Branch if negative flag is set
	uint8_t offset = ReadNextByte();
	if (m_p & FLAG_NEGATIVE) {
		if (NearBranch(offset))
			m_cycle_count++; // Extra cycle for page crossing
		m_cycle_count += 3; // Branch taken
	}
	else {
		m_cycle_count += 2; // Branch not taken
	}
}

void CPU::BRK() {
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
			if (operand == 9) {
				int i = 0;
			}
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
		case PHP_IMPLIED:
		{
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
	}
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

inline uint8_t CPU::ReadByte(uint16_t addr)
{
	return bus->read(addr);
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

void CPU::ADC()
{
	uint8_t a_old = m_a;
	
	uint16_t result = m_a + _operand + (m_p & FLAG_CARRY ? 1 : 0);
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
	if (((a_old ^ m_a) & (_operand ^ m_a) & 0x80) != 0) {
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

void CPU::AND()
{
	// AND operation with the accumulator
	m_a &= _operand;
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

void CPU::ASL()
{
	// Set/clear carry flag based on bit 7
	if (_operand & 0x80) {
		m_p |= FLAG_CARRY;   // Set carry
	}
	else {
		m_p &= ~FLAG_CARRY;  // Clear carry
	}
	// Shift left by 1
	_operand <<= 1;
	// Set/clear zero flag
	if (_operand == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)
	if (_operand & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
}

bool CPU::NearBranch(uint8_t value) {
	int8_t offset = static_cast<int8_t>(value); // Convert to signed
	// Determine if page crossed
	uint8_t old_pc_hi = (m_pc & 0xFF00) >> 8;
	m_pc += offset;
	uint8_t new_pc_hi = (m_pc & 0xFF00) >> 8;
	return old_pc_hi != new_pc_hi;
}

void CPU::BCC() {
	if (!(m_p & FLAG_CARRY)) {
		if (NearBranch(_operand))
			m_cycle_count++; // Extra cycle for page crossing
		m_cycle_count += 3; // Branch taken
	}
	else {
		m_cycle_count += 2; // Branch not taken
	}
}

void CPU::BCS() {
	dbg(L"0x%02X", _operand);
	if (m_p & FLAG_CARRY) {
		if (NearBranch(_operand))
			m_cycle_count++; // Extra cycle for page crossing
		m_cycle_count += 3; // Branch taken
	}
	else {
		m_cycle_count += 2; // Branch not taken
	}
}

void CPU::BEQ() {
	uint8_t offset = ReadNextByte();
	dbg(L"0x%02X", offset);
	if (m_p & FLAG_ZERO) {
		if (NearBranch(offset))
			m_cycle_count++; // Extra cycle for page crossing
		m_cycle_count += 3; // Branch taken
	}
	else {
		m_cycle_count += 2; // Branch not taken
	}
}

void CPU::BIT() {
	// Set zero flag based on AND with accumulator
	if ((m_a & _operand) == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set negative flag based on bit 7 of data
	if (_operand & 0x80) {
		m_p |= FLAG_NEGATIVE;
	}
	else {
		m_p &= ~FLAG_NEGATIVE;
	}
	// Set overflow flag based on bit 6 of data
	if (_operand & 0x40) {
		m_p |= FLAG_OVERFLOW;
	}
	else {
		m_p &= ~FLAG_OVERFLOW;
	}
}

void CPU::BNE() {
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
}

void CPU::BPL() {
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
}

void CPU::BVC() {
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
}

void CPU::BVS() {
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
}

void CPU::CLI() {
	m_p &= ~FLAG_INTERRUPT; // Clear interrupt disable flag
	m_cycle_count += 2;
}

void CPU::CLV() {
	m_p &= ~FLAG_OVERFLOW; // Clear overflow flag
	m_cycle_count += 2;
}

void CPU::CLC() {
	// Clear carry flag
	m_p &= ~FLAG_CARRY;
	m_cycle_count += 2;
}

void CPU::CLD() {
	// Clear decimal mode flag
	m_p &= ~FLAG_DECIMAL;
	m_cycle_count += 2;
}

void CPU::CMP() {
	cp(m_a, _operand);
	m_cycle_count += 2;
}

void CPU::cp(uint8_t value, uint8_t operand)
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

void CPU::CPX() {
	cp(m_x, _operand);
	m_cycle_count += 2;
}

void CPU::CPY () {
	cp(m_y, _operand);
	m_cycle_count += 2;
}

void CPU::DEC() {
	uint8_t data = ReadByte(_operand);
	data--;
	bus->write(_operand, data);
	SetZero(data);
	SetNegative(data);
	m_cycle_count += 5;
}

void CPU::DEX() {
	m_x--;
	// Set/clear zero flag
	SetZero(m_x);
	SetNegative(m_x);
	m_cycle_count += 2;
}

void CPU::DEY() {
	m_y--;
	// Set/clear zero flag
	SetZero(m_y);
	SetNegative(m_y);
	m_cycle_count += 2;
}

void CPU::DMP() {
	// Dummy instruction for illegal opcodes
	// Does nothing, just consumes cycles
	m_pc -= 1; // Step back to re-read the opcode
}

void CPU::EOR()
{
	// EOR operation with the accumulator
	m_a ^= _operand;
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

void CPU::INC() {
	uint8_t data = ReadByte(_operand);
	data++;
	bus->write(_operand, data);
	SetZero(data);
	SetNegative(data);
	m_cycle_count += 5;
}

void CPU::INX() {
	m_x++;
	// Set/clear zero flag
	SetZero(m_x);
	SetNegative(m_x);
	m_cycle_count += 2;
}

void CPU::INY() {
	m_y++;
	// Set/clear zero flag
	SetZero(m_y);
	SetNegative(m_y);
	m_cycle_count += 2;
}

void CPU::JMP(uint16_t addr) {
	m_pc = addr;
	dbg(L"JMP to 0x%04X", m_pc);
	m_cycle_count += 3;
}

void CPU::JSR() {
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
}

void CPU::LDA() {
	m_a = _operand;
	SetZero(m_a);
	SetNegative(m_a);
	m_cycle_count += 2;
}

void CPU::LDX() {
	m_x = _operand;
	SetZero(m_x);
	SetNegative(m_x);
	m_cycle_count += 2;
}

void CPU::LDY() {
	m_y = _operand;
	SetZero(m_y);
	SetNegative(m_y);
	m_cycle_count += 2;
}

void CPU::LSR()
{
	// Set/clear carry flag based on bit 0  
	if (_operand & 0x01) {
		m_p |= FLAG_CARRY;   // Set carry  
	}
	else {
		m_p &= ~FLAG_CARRY;  // Clear carry  
	}
	// Shift right by 1
	_operand >>= 1;
	dbg(L"0x%02X", _operand);
	// Set/clear zero flag  
	if (_operand == 0) {
		m_p |= FLAG_ZERO;
	}
	else {
		m_p &= ~FLAG_ZERO;
	}
	// Set/clear negative flag (bit 7 of result)  
	m_p &= ~FLAG_NEGATIVE; // Clear negative flag since result of LSR is always positive  
}

void CPU::NOP() {
	// No operation
	m_cycle_count += 2;
}

void CPU::ORA()
{
	// Perform bitwise OR operation with the accumulator  
	m_a |= _operand;
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

void CPU::PHA() {
	bus->write(0x0100 + m_sp--, m_a);
	m_cycle_count += 3;
}

void CPU::PLA() {
	m_a = ReadByte(0x0100 + ++m_sp);
	SetZero(m_a);
	SetNegative(m_a);
	m_cycle_count += 4;
}

void CPU::PHP() {
	bus->write(0x0100 + m_sp--, m_p | FLAG_BREAK | 0x20); // Set B and unused flag bits when pushing P
	m_cycle_count += 3;
}

void CPU::PLP() {
	bool break_flag = (m_p & FLAG_BREAK) != 0; // Save current B flag state
	m_p = ReadByte(0x0100 + ++m_sp);
	SetBreak(break_flag); // Restore B flag state
	m_cycle_count += 4;
}

void CPU::ROL()
{  
	// Save the carry flag before modifying it  
	bool carry_in = (m_p & FLAG_CARRY) != 0;  

	// Set/clear carry flag based on bit 7  
	if (_operand & 0x80) {  
		m_p |= FLAG_CARRY;   // Set carry  
	} else {  
		m_p &= ~FLAG_CARRY;  // Clear carry  
	}  

	// Rotate left by 1 (shift left and insert carry into bit 0)  
	_operand = (_operand << 1) | (carry_in ? 1 : 0);

	// Set/clear zero flag  
	SetZero(_operand);

	// Set/clear negative flag (bit 7 of result)  
	SetNegative(_operand);
}

void CPU::ROR()
{
	// Save the carry flag before modifying it  
	bool carry_in = (m_p & FLAG_CARRY) != 0;  
	// Set/clear carry flag based on bit 0  
	if (_operand & 0x01) {
		m_p |= FLAG_CARRY;   // Set carry  
	} else {  
		m_p &= ~FLAG_CARRY;  // Clear carry  
	}  
	// Rotate right by 1 (shift right and insert carry into bit 7)  
	_operand = (_operand >> 1) | (carry_in ? 0x80 : 0);
	// Set/clear zero flag  
	SetZero(_operand);
	// Set/clear negative flag (bit 7 of result)  
	SetNegative(_operand);
}

void CPU::RTI() {
	// Pull P from stack
	dbgNmi(L"\nRTI (cycle %d)\n", m_cycle_count);
	uint8_t pulledP = ReadByte(0x0100 + ++m_sp);
	dbg(L"Readomg 0x%02X from stack 0x%02X\n", pulledP, m_sp - 1);

	m_p = static_cast<uint8_t>((pulledP & ~FLAG_BREAK) | FLAG_UNUSED);

	// Pull PC from stack (low byte first)
	uint8_t pc_lo = ReadByte(0x0100 + ++m_sp);
	dbg(L"Readomg 0x%02X from stack 0x%02X\n", pc_lo, m_sp - 1);
	uint8_t pc_hi = ReadByte(0x0100 + ++m_sp);
	dbg(L"Readomg 0x%02X from stack 0x%02X\n", pc_hi, m_sp - 1);
	m_pc = (static_cast<uint16_t>(pc_hi << 8) | pc_lo);
	dbg(L"Setting PC to 0x%04X\n", m_pc);
	m_cycle_count += 6;
}

void CPU::RTS() {
	// Pull return address from stack (low byte first)
	uint8_t pc_lo = ReadByte(0x0100 + ++m_sp);
	dbg(L"Pulling 0x%02X from stack 0x%02X\n", pc_lo, m_sp);
	uint8_t pc_hi = ReadByte(0x0100 + ++m_sp);
	dbg(L"Pulling 0x%02X from stack 0x%02X\n", pc_hi, m_sp);
	m_pc = (static_cast<uint16_t>(pc_hi << 8) | pc_lo);
	m_pc++; // Increment PC to point to the next instruction after the JSR
	dbg(L"Changing pc to 0x%04X\n", m_pc);
	m_cycle_count += 6;
}

void CPU::SBC()
{
	// Invert the operand for subtraction
	_operand = ~_operand;
	// Perform addition with inverted operand and carry flag
	ADC();
}

void CPU::SEC() {
	// Set carry flag
	m_p |= FLAG_CARRY;
	m_cycle_count += 2;
}

void CPU::SED() {
	// Set decimal mode flag
	m_p |= FLAG_DECIMAL;
	m_cycle_count += 2;
}

void CPU::SEI() {
	// Set interrupt disable flag
	m_p |= FLAG_INTERRUPT;
	m_cycle_count += 2;
}

void CPU::STA() {
	bus->write(_operand, m_a);
	m_cycle_count += 3;
}

void CPU::STX() {
	bus->write(_operand, m_x);
	m_cycle_count += 3;
}

void CPU::STY() {
	bus->write(_operand, m_y);
	m_cycle_count += 3;
}

void CPU::TAX() {
	m_x = m_a;
	SetZero(m_x);
	SetNegative(m_x);
	m_cycle_count += 2;
}

void CPU::TAY() {
	m_y = m_a;
	SetZero(m_y);
	SetNegative(m_y);
	m_cycle_count += 2;
}

void CPU::TSX() {
	m_x = m_sp;
	dbg(L"Transferring sp 0x%02X to x\n", m_sp);
	SetZero(m_x);
	SetNegative(m_x);
	m_cycle_count += 2;
}

void CPU::TXA() {
	m_a = m_x;
	SetZero(m_a);
	SetNegative(m_a);
	m_cycle_count += 2;
}

void CPU::TXS() {
	m_sp = m_x;
	m_cycle_count += 2;
}

void CPU::TYA() {
	m_a = m_y;
	SetZero(m_a);
	SetNegative(m_a);
	m_cycle_count += 2;
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