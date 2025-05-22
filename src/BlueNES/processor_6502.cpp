#include "processor_6502.h"

/* We emulate the 6502 only as far as it is compatible with the NES. For example, we do not include Decimal Mode.*/

// Program counter
int m_pc;

// Registers
uint8_t m_a;
uint8_t m_x;
uint8_t m_y;

// flags
bool m_carry;
bool m_zero;
bool m_interruptDisable;
bool m_breakCommand;
bool m_overflow;
bool m_negative;

uint8_t* m_pRomData;

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html
// op codes
const uint8_t adc_immediate = 0x69;

//void Processor_6502::IncrementPC()
//{
//	m_pc++;
//}

void Processor_6502::Initialize(uint8_t* romData)
{
	m_pRomData = romData;
	m_pc = 0;
}

void Processor_6502::Run()
{
	while (true)
	{
		switch (m_pRomData[m_pc])
		{
			case adc_immediate:

			break;
		}
	}
}

// Primarily used for testing purposes.
uint8_t Processor_6502::GetA()
{
	return m_a;
}

uint8_t Processor_6502::GetX()
{
	return m_x;
}

uint8_t Processor_6502::GetY()
{
	return m_y;
}