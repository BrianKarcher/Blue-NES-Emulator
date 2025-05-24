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

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html
// op codes
const uint8_t adc_immediate = 0x69;
const uint8_t adc_zeropage = 0x65;

//void Processor_6502::IncrementPC()
//{
//	m_pc++;
//}

void Processor_6502::Initialize(uint8_t* romData, uint8_t* memory)
{
	m_pRomData = romData;
	m_pMemory = memory;
	m_pc = 0;
	m_p = 0;
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
	switch (m_pRomData[m_pc])
	{
		case adc_immediate:
			// Immediate mode gets the data from ROM, not RAM. It is the next byte after the op code.
			adc(m_pRomData[m_pc + 1]);
			m_pc += 2;
			break;
		case adc_zeropage:
			// The byte after the op code is the zero page address.
			adc(m_pMemory[m_pc + 1]);
			m_pc += 2;
			break;
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