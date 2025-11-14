#include "CPU.h"
#include "Bus.h"

/* We emulate the 6502 only as far as it is compatible with the NES. For example, we do not include Decimal Mode.*/

// Reference https://www.nesdev.org/obelisk-6502-guide/reference.html
int cnt = 0;
int cnt2 = 0;

void Processor_6502::PowerOn()
{
	m_pc = (static_cast<uint16_t>(bus->read(0xFFFD) << 8)) | bus->read(0xFFFC);
	m_p = 0;
	m_a = 0;
	m_x = 0;
	m_y = 0;
	m_sp = 0xFD;
	m_cycle_count = 0;
	isActive = true;
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
	uint8_t op = bus->read(m_pc++);
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
			m_p |= FLAG_BREAK; // Set break flag
			m_pc += 2; // Increment PC by 2 to skip over the next byte (padding byte)
			// Push PC and P to stack
			bus->write(0x0100 + m_sp--, (m_pc >> 8) & 0xFF); // Push high byte of PC
			bus->write(0x0100 + m_sp--, m_pc & 0xFF);        // Push low byte of PC
			bus->write(0x0100 + m_sp--, m_p);                // Push processor status
			// Set PC to NMI vector
			m_pc = (static_cast<uint16_t>(ReadByte(0xFFFF) << 8)) | ReadByte(0xFFFE);
			// NMI takes 7 cycles
			m_cycle_count += 7;
			break;
		}
		case BVC_RELATIVE:
		{
			uint8_t offset = ReadNextByte();
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
			cnt += 1;
			std::wstring msg = L"DEY\n" + std::to_wstring(cnt);
			OutputDebugString(msg.c_str());
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
			m_pc = (static_cast<uint16_t>(hiByte << 8) | loByte);
			m_cycle_count += 5;
			break;
		}
		case JSR_ABSOLUTE:
		{
			uint16_t return_addr = m_pc + 1; // Address to return to after subroutine
			// Push return address onto stack (high byte first)
			bus->write(0x0100 + m_sp--, (return_addr >> 8) & 0xFF); // High byte
			bus->write(0x0100 + m_sp--, return_addr & 0xFF);        // Low byte
			// Set PC to target address
			m_pc = ReadNextWord();
			m_cycle_count += 6;
			break;
		}
		case LDA_IMMEDIATE:
		{
			uint8_t operand = ReadNextByte();
			m_a = operand;
			SetZero(m_a);
			SetNegative(m_a);
			m_cycle_count += 2;
			break;
		}
		case LDA_ZEROPAGE:
		{
			uint8_t operand = ReadByte(ReadNextByte());
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
			bool break_flag = (m_p & FLAG_BREAK) != 0; // Save current B flag state
			// Pull P from stack
			m_p = ReadByte(0x0100 + ++m_sp);
			SetBreak(break_flag); // Restore B flag state
			// Pull PC from stack (low byte first)
			uint8_t pc_lo = ReadByte(0x0100 + ++m_sp);
			uint8_t pc_hi = ReadByte(0x0100 + ++m_sp);
			m_pc = (static_cast<uint16_t>(pc_hi << 8) | pc_lo);
			m_cycle_count += 6;
			break;
		}
		case RTS_IMPLIED:
		{
			// Pull return address from stack (low byte first)
			uint8_t pc_lo = ReadByte(0x0100 + ++m_sp);
			uint8_t pc_hi = ReadByte(0x0100 + ++m_sp);
			m_pc = (static_cast<uint16_t>(pc_hi << 8) | pc_lo);
			m_pc++; // Increment PC to point to the next instruction after the JSR
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
			if (addr >= 0x2000 && addr <= 0x2FFF) {
				int i = 0;
			}
			if (addr == 0x2007) {
				cnt2 += 1;
				std::wstring msg = L"STA\n" + std::to_wstring(cnt2);
				OutputDebugString(msg.c_str());
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
	uint8_t zp_base = bus->read(m_pc++);
	// Add X register to base (with zp wraparound)
	uint8_t zp_addr = (zp_base + m_x) & 0xFF;
	uint8_t addr_lo = bus->read(zp_addr);
	uint8_t addr_hi = bus->read((zp_addr + 1) & 0xFF); // Wraparound for the high byte
	return (addr_hi << 8) | addr_lo;
}

inline uint16_t Processor_6502::ReadIndirectIndexed()
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
	return (addr_hi << 8) | addr_lo;
}

inline uint16_t Processor_6502::ReadIndirectIndexedNoCycle()
{
	uint8_t zp_base = bus->read(m_pc++);
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