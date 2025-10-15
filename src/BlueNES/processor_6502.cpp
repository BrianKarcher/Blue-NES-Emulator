#include "processor_6502.h"

/* We emulate the 6502 only as far as it is compatible with the NES. For example, we do not include Decimal Mode.*/

// Program counter
int m_pc;

// Registers
uint8_t m_a;
uint8_t m_x;
uint8_t m_y;

// flags
uint8_t m_p;

uint8_t* m_pRomData;
uint8_t* m_pMemory;
int m_cycle_count = 0;

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html

void Processor_6502::Initialize(uint8_t* romData, uint8_t* memory)
{
	m_pRomData = romData;
	m_pMemory = memory;
	m_pc = 0;
	m_p = 0;
	m_cycle_count = 0;
}

void Processor_6502::Run()
{
	while (true)
	{
		RunStep();
	}
}

/// <summary>
/// Speed is paramount so we try to reduce function hops as much as feasible while keeping the code readable.
/// And yes, I realize that the loop in the Run() function is an extra hop to RunStep(). I'm keeping that hop to
/// make testing easier.
/// </summary>
void Processor_6502::RunStep()
{
	switch (m_pRomData[m_pc++])
	{
		case ADC_IMMEDIATE:
			// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
			adc(m_pRomData[m_pc++]);
			m_cycle_count += 2;
			break;
		case ADC_ZEROPAGE:
			// An instruction using zero page addressing mode has only an 8 bit address operand.
			// This limits it to addressing only the first 256 bytes of memory (e.g. $0000 to $00FF)
			// where the most significant byte of the address is always zero.
			adc(m_pMemory[m_pRomData[m_pc++]]);
			m_cycle_count += 3;
			break;
		case ADC_ZEROPAGE_X:
		{
			// The address to be accessed by an instruction using indexed zero page
			// addressing is calculated by taking the 8 bit zero page address from the
			// instruction and adding the current value of the X register to it.
			// The address calculation wraps around if the sum of the base address and the register exceed $FF.
			uint8_t offset = m_pMemory[m_pRomData[m_pc++] + m_x];
			adc(offset);
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE:
		{
			uint8_t loByte = m_pRomData[m_pc++];
			uint8_t hiByte = m_pRomData[m_pc++];
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			adc(m_pMemory[memoryLocation]);
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
			uint8_t loByte = m_pRomData[m_pc++];
			uint8_t hiByte = m_pRomData[m_pc++];
			loByte += m_x;
			// Carryover?
			if (loByte < m_x)
			{
				hiByte += 1;
				// One more cycle if a page is crossed.
				m_cycle_count++;
			}
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			adc(m_pMemory[memoryLocation]);
			m_cycle_count += 4;
			break;
		}
		case ADC_ABSOLUTE_Y:
		{
			uint8_t loByte = m_pRomData[m_pc++];
			uint8_t hiByte = m_pRomData[m_pc++];
			loByte += m_y;
			if (loByte < m_y)
			{
				hiByte += 1; // Carryover
				m_cycle_count++; // Extra cycle for page crossing
			}
			uint16_t memoryLocation = (static_cast<uint16_t>(hiByte << 8) | loByte);
			adc(m_pMemory[memoryLocation]);
			m_cycle_count += 4;
			break;
		}
		case ADC_INDEXEDINDIRECT:
		{
			uint8_t zp_base = m_pRomData[m_pc++];
			// Add X register to base (with zp wraparound)
			uint8_t zp_addr = (zp_base + m_x) & 0xFF;
			uint8_t addr_lo = m_pMemory[zp_addr];
			uint8_t addr_hi = m_pMemory[(zp_addr + 1) & 0xFF]; // Wraparound for the high byte
			uint16_t target_addr = (addr_hi << 8) | addr_lo;

			uint8_t operand = m_pMemory[target_addr];
			adc(operand);
			m_cycle_count += 6;
			break;
		}
		case ADC_INDIRECTINDEXED:
		{
			uint8_t zp_base = m_pRomData[m_pc++];
			uint8_t addr_lo = m_pMemory[zp_base];
			uint8_t addr_hi = m_pMemory[(zp_base + 1) & 0xFF]; // Wraparound for the high byte
			// Add Y register to the low byte of the address
			addr_lo += m_y;
			if (addr_lo < m_y) {
				addr_hi += 1; // Carryover
				m_cycle_count++; // Extra cycle for page crossing
			}
			uint16_t target_addr = (addr_hi << 8) | addr_lo;
			uint8_t operand = m_pMemory[target_addr];
			adc(operand);
			m_cycle_count += 5;
			break;
		}
		//case AND_IMMEDIATE:
		//{
		//	// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
		//	uint8_t operand = m_pRomData[m_pc++];
		//	_and(operand);
		//	m_cycle_count += 2;
		//	break;
		//}
		//case AND_ZEROPAGE:
		//{
		//	// An instruction using zero page addressing mode has only an 8 bit address operand.
		//	// This limits it to addressing only the first 256 bytes of memory (e.g. $0000 to $00FF)
		//	// where the most significant byte of the address is always zero.
		//	uint8_t operand = m_pMemory[m_pRomData[m_pc++]];
		//	_and(operand);
		//	m_cycle_count += 3;
		//	break;
		//}
		//case AND_ZEROPAGE_X:
		//{
		//	// The address to be accessed by an instruction using indexed zero page
		//	// addressing is calculated by taking the 8 bit zero page address from the
		//	// instruction and adding the current value of the X register to it.
		//	// The address calculation wraps around if the sum of the base address and the register exceed $FF.
		//	uint8_t operand = m_pMemory[m_pRomData[m_pc++] + m_x];
		//	_and(operand);
		//	m_cycle_count += 4;
		//	break;
		//}
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

void _and(uint8_t operand)
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
	return (m_p & flag) == 1;
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