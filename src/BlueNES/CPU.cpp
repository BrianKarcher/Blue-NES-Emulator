#include "CPU.h"
#include "Bus.h"

/* We emulate the 6502 only as far as it is compatible with the NES. For example, we do not include Decimal Mode.*/

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html

void Processor_6502::Initialize()
{
	m_pc = 0x8000;
	m_p = 0;
	m_cycle_count = 0;
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
	m_cycle_count = 0;
}

void Processor_6502::AddCycles(int count)
{
	m_cycle_count += count;
}

uint64_t Processor_6502::GetCycleCount()
{
	return m_cycle_count;
}

void Processor_6502::NMI() {
	// Push PC and P to stack
	bus->write(0x0100 + m_sp--, (m_pc >> 8) & 0xFF); // Push high byte of PC
	bus->write(0x0100 + m_sp--, m_pc & 0xFF);        // Push low byte of PC
	bus->write(0x0100 + m_sp--, m_p);                 // Push processor status
	// Set PC to NMI vector
	m_pc = (static_cast<uint16_t>(bus->read(0xFFFB) << 8)) | bus->read(0xFFFA);
	// NMI takes 7 cycles
	m_cycle_count += 7;
}

/// <summary>
/// Speed is paramount so we try to reduce function hops as much as feasible while keeping the code readable.
/// And yes, I realize that the loop in the Run() function is an extra hop to RunStep(). I'm keeping that hop to
/// make testing easier.
/// </summary>
void Processor_6502::Clock()
{
	if (!isActive) return;
	switch (bus->read(m_pc++))
	{
		case ADC_IMMEDIATE:
			// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
			adc(bus->read(m_pc++));
			m_cycle_count += 2;
			break;
		case ADC_ZEROPAGE:
			// An instruction using zero page addressing mode has only an 8 bit address operand.
			// This limits it to addressing only the first 256 bytes of memory (e.g. $0000 to $00FF)
			// where the most significant byte of the address is always zero.
			adc(bus->read(bus->read(m_pc++)));
			m_cycle_count += 3;
			break;
		case ADC_ZEROPAGE_X:
		{
			// The address to be accessed by an instruction using indexed zero page
			// addressing is calculated by taking the 8 bit zero page address from the
			// instruction and adding the current value of the X register to it.
			// The address calculation wraps around if the sum of the base address and the register exceed $FF.
			uint8_t offset = bus->read(bus->read(m_pc++) + m_x);
			adc(offset);
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE:
		{
			uint8_t loByte = bus->read(m_pc++);
			uint8_t hiByte = bus->read(m_pc++);
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			adc(bus->read(memoryLocation));
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
			uint8_t loByte = bus->read(m_pc++);
			uint8_t hiByte = bus->read(m_pc++);
			loByte += m_x;
			// Carryover?
			if (loByte < m_x)
			{
				hiByte += 1;
				// One more cycle if a page is crossed.
				m_cycle_count++;
			}
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			adc(bus->read(memoryLocation));
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE_Y:
		{
			uint8_t loByte = bus->read(m_pc++);
			uint8_t hiByte = bus->read(m_pc++);
			loByte += m_y;
			if (loByte < m_y)
			{
				hiByte += 1; // Carryover
				m_cycle_count++; // Extra cycle for page crossing
			}
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			adc(bus->read(memoryLocation));
			m_cycle_count += 4;
			break;
		}
		case ADC_INDEXEDINDIRECT:
		{
			uint8_t zp_base = bus->read(m_pc++);
			// Add X register to base (with zp wraparound)
			uint8_t zp_addr = (zp_base + m_x) & 0xFF;
			uint8_t addr_lo = bus->read(zp_addr);
			uint8_t addr_hi = bus->read((zp_addr + 1) & 0xFF); // Wraparound for the high byte
			uint16_t target_addr = (addr_hi << 8) | addr_lo;

			uint8_t operand = bus->read(target_addr);
			adc(operand);
			m_cycle_count += 6;
			break;
		}
		case ADC_INDIRECTINDEXED:
		{
			uint8_t zp_base = bus->read(m_pc++);
			uint8_t addr_lo = bus->read(zp_base);
			uint8_t addr_hi = bus->read((zp_base + 1) & 0xFF); // Wraparound for the high byte
			// Add Y register to the low byte of the address
			addr_lo += m_y;
			if (addr_lo < m_y) {
				addr_hi += 1; // Carryover
				m_cycle_count++; // Extra cycle for page crossing
			}
			uint16_t target_addr = (addr_hi << 8) | addr_lo;
			uint8_t operand = bus->read(target_addr);
			adc(operand);
			m_cycle_count += 5;
			break;
		}
		case AND_IMMEDIATE:
		{
			// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
			uint8_t operand = bus->read(m_pc++);
			_and(operand);
			m_cycle_count += 2;
			break;
		}
		case AND_ZEROPAGE:
		{
			// An instruction using zero page addressing mode has only an 8 bit address operand.
			// This limits it to addressing only the first 256 bytes of memory (e.g. $0000 to $00FF)
			// where the most significant byte of the address is always zero.
			uint8_t operand = bus->read(bus->read(m_pc++));
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
			uint8_t operand = bus->read(bus->read(m_pc++) + m_x);
			_and(operand);
			m_cycle_count += 4;
			break;
		}
	}
}

void Processor_6502::adc(uint8_t operand)
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

bool Processor_6502::GetFlag(uint8_t flag)
{
	return (m_p & flag) == flag;
}

void Processor_6502::SetFlag(bool byte, uint8_t flag)
{
	if (byte) {
		m_p |= flag;   // Set flag
	}
	else {
		m_p &= ~flag;  // Clear flag
	}
}