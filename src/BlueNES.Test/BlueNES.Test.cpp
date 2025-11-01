#include <cstdlib>
#include "pch.h"
#include "CppUnitTest.h"
#include "cpu.h"
#include "Cartridge.h"
#include "Bus.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BlueNESTest
{
	TEST_CLASS(BlueNESTest)
	{
	private:
		Processor_6502 processor;
		Bus bus;
		Cartridge cart;

	public:
		TEST_METHOD_INITIALIZE(TestSetup)
		{
			bus.cart = &cart;
			bus.cpu = &processor;
			processor.bus = &bus;
			processor.Initialize();
			processor.Activate(true);
		}

		TEST_METHOD(TestADCImmediate1)
		{
			uint8_t rom[] = { ADC_IMMEDIATE, 0x20 };
			cart.SetPRGRom(rom, sizeof(rom));

			processor.Clock();
			Assert::AreEqual((uint8_t)0x20, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCImmediateWithCarry)
		{
			uint8_t rom[] = { ADC_IMMEDIATE, 0x20 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0x11);
			processor.SetFlag(true, FLAG_CARRY);
			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x32, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCImmediateWithOverflow)
		{
			// $ef + $20 will result in overflow
			processor.SetA(0xef);
			uint8_t rom[] = { ADC_IMMEDIATE, 0x20 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.Clock();
			Assert::AreEqual((uint8_t)0xf, processor.GetA());
			Assert::IsTrue(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCZeroPage)
		{
			// Add what is at zero page 0x15 to A.
			uint8_t rom[] = { ADC_ZEROPAGE, 0x15 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x0015, 0x69);
			processor.SetA(0x18);
			processor.Clock();
			Assert::AreEqual((uint8_t)0x81, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCZeroPage_X)
		{
			// Add what is at zero page 0x15 to A.
			uint8_t rom[] = { ADC_ZEROPAGE_X, 0x15 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x0016, 0x69);
			processor.SetA(0x18);
			processor.SetX(0x1);
			processor.Clock();
			Assert::AreEqual((uint8_t)0x81, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCAbsolute)
		{
			// I think the 6502 stores in little endian, need to double check
			uint8_t rom[] = { ADC_ABSOLUTE, 0x23, 0x3 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x323, 0x40);
			processor.SetA(0x20);
			processor.Clock();
			Assert::AreEqual((uint8_t)0x60, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCNonNegative)
		{
			// I think the 6502 stores in little endian, need to double check
			uint8_t rom[] = { ADC_ABSOLUTE, 0x23, 0x3 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x323, 0x40);
			processor.SetA(0x20);
			processor.SetFlag(true, FLAG_ZERO);
			processor.Clock();
			Assert::AreEqual((uint8_t)0x60, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
			Assert::IsFalse(processor.GetFlag(FLAG_ZERO));
		}
		TEST_METHOD(TestADCImmediateNegativeResult)
		{
			uint8_t rom[] = { ADC_IMMEDIATE, 0x90 }; // 144 decimal
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0x50); // 80 decimal
			processor.Clock();
			// 80 + 144 = 224 which is negative in signed 8-bit
			Assert::AreEqual((uint8_t)0xE0, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
			Assert::IsTrue(processor.GetFlag(FLAG_NEGATIVE));
		}

		TEST_METHOD(TestANDImmediate)
		{
			uint8_t rom[] = { AND_IMMEDIATE, 0x7 }; // 0111
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110
			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestANDZeroPage)
		{
			bus.write(0x0035, 0x07); // 0111
			uint8_t rom[] = { AND_ZEROPAGE, 0x35 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110

			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestANDZeroPageX)
		{
			bus.write(0x0035, 0x07); // 0111
			uint8_t rom[] = { AND_ZEROPAGE_X, 0x34 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetX(0x1);
			processor.SetA(0xE); // 1110

			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestANDAbsolute)
		{
			bus.write(0x0235, 0x07); // 0111
			uint8_t rom[] = { AND_ABSOLUTE, 0x35, 0x02 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110

			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestANDAbsoluteX)
		{
			bus.write(0x0235, 0x07); // 0111
			uint8_t rom[] = { AND_ABSOLUTE_X, 0x33, 0x02 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110
			processor.SetX(0x2);

			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestANDAbsoluteY)
		{
			bus.write(0x0235, 0x07); // 0111
			uint8_t rom[] = { AND_ABSOLUTE_Y, 0x33, 0x02 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110
			processor.SetY(0x2);

			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestANDIndexedIndirect)
		{
			bus.write(0x0035, 0x35);
			bus.write(0x0036, 0x02); // Pointer to 0x0235
			bus.write(0x0235, 0x07); // 0111
			uint8_t rom[] = { AND_INDEXEDINDIRECT, 0x33 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110
			processor.SetX(0x2);

			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}
		TEST_METHOD(TestANDIndirectIndexed)
		{
			bus.write(0x0035, 0x35);
			bus.write(0x0036, 0x02); // Pointer to 0x0235
			bus.write(0x0237, 0x07); // 0111
			uint8_t rom[] = { AND_INDIRECTINDEXED, 0x35 };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0xE); // 1110
			processor.SetY(0x2);
			processor.Clock();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x6, processor.GetA()); // 0110
		}

		TEST_METHOD(TestASLAccumulator)
		{
			uint8_t rom[] = { ASL_ACCUMULATOR };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetA(0x45); // 0100 0101
			processor.Clock();
			// Shift left: 1000 1010
			Assert::AreEqual((uint8_t)0x8A, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}
		TEST_METHOD(TestASLZeroPage)
		{
			uint8_t rom[] = { ASL_ZEROPAGE, 0x10 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x0010, 0x45); // 0100 0101
			processor.Clock();
			// Shift left: 1000 1010
			Assert::AreEqual((uint8_t)0x8A, bus.read(0x0010));
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}
		TEST_METHOD(TestASLZeroPageX)
		{
			uint8_t rom[] = { ASL_ZEROPAGE_X, 0x0F };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x0010, 0x45); // 0100 0101
			processor.SetX(0x1);
			processor.Clock();
			// Shift left: 1000 1010
			Assert::AreEqual((uint8_t)0x8A, bus.read(0x0010));
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}
		TEST_METHOD(TestASLAbsolute)
		{
			uint8_t rom[] = { ASL_ABSOLUTE, 0x25, 0x15 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x1525, 0x45); // 0100 0101
			processor.Clock();
			// Shift left: 1000 1010
			Assert::AreEqual((uint8_t)0x8A, bus.read(0x1525));
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}
		TEST_METHOD(TestASLAbsoluteX)
		{
			uint8_t rom[] = { ASL_ABSOLUTE_X, 0x24, 0x15 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x1525, 0x45); // 0100 0101
			processor.SetX(0x1);
			processor.Clock();
			// Shift left: 1000 1010
			Assert::AreEqual((uint8_t)0x8A, bus.read(0x1525));
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}
		TEST_METHOD(TestBCCRelative)
		{
			uint8_t rom[] = { BCC_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(false, FLAG_CARRY); // Clear carry to take branch
			processor.Clock();
			// After clocking BCC, PC should be at 0x8007 (start at 0x8000 + 2 for instruction + 5 for branch)
			Assert::AreEqual((uint16_t)0x8007, processor.GetPC());
		}
		TEST_METHOD(TestBCCRelativeNotTaken)
		{
			uint8_t rom[] = { BCC_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(true, FLAG_CARRY); // Set carry to not take branch
			processor.Clock();
			// After clocking BCC, PC should be at 0x8002 (start at 0x8000 + 2 for instruction)
			Assert::AreEqual((uint16_t)0x8002, processor.GetPC());
		}
		TEST_METHOD(TestBCSRelative)
		{
			uint8_t rom[] = { BCS_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(true, FLAG_CARRY); // Set carry to take branch
			processor.Clock();
			// After clocking BCS, PC should be at 0x8007 (start at 0x8000 + 2 for instruction + 5 for branch)
			Assert::AreEqual((uint16_t)0x8007, processor.GetPC());
		}
		TEST_METHOD(TestBCSRelativeNotTaken)
		{
			uint8_t rom[] = { BCS_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(false, FLAG_CARRY); // Clear carry to not take branch
			processor.Clock();
			// After clocking BCS, PC should be at 0x8002 (start at 0x8000 + 2 for instruction)
			Assert::AreEqual((uint16_t)0x8002, processor.GetPC());
		}
		TEST_METHOD(TestBEQRelative)
		{
			uint8_t rom[] = { BEQ_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(true, FLAG_ZERO); // Set zero to take branch
			processor.Clock();
			// After clocking BEQ, PC should be at 0x8007 (start at 0x8000 + 2 for instruction + 5 for branch)
			Assert::AreEqual((uint16_t)0x8007, processor.GetPC());
		}
		TEST_METHOD(TestBEQRelativeNotTaken)
		{
			uint8_t rom[] = { BEQ_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(false, FLAG_ZERO); // Clear zero to not take branch
			processor.Clock();
			// After clocking BEQ, PC should be at 0x8002 (start at 0x8000 + 2 for instruction)
			Assert::AreEqual((uint16_t)0x8002, processor.GetPC());
		}
		TEST_METHOD(TestBITZeroPage)
		{
			uint8_t rom[] = { BIT_ZEROPAGE, 0x10 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x0010, 0b11000000); // Set bits 6 and 7
			processor.SetA(0b11111111); // A can be anything, just testing flags
			processor.Clock();
			Assert::IsTrue(processor.GetFlag(FLAG_NEGATIVE)); // Bit 7 set
			Assert::IsTrue(processor.GetFlag(FLAG_OVERFLOW)); // Bit 6 set
			Assert::IsFalse(processor.GetFlag(FLAG_ZERO));    // A & M != 0
		}
		TEST_METHOD(TestBITAbsolute)
		{
			uint8_t rom[] = { BIT_ABSOLUTE, 0x10, 0x15 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x1510, 0b01000000); // Set bit 6
			processor.SetA(0b11111111); // A can be anything, just testing flags
			processor.Clock();
			Assert::IsFalse(processor.GetFlag(FLAG_NEGATIVE)); // Bit 7 clear
			Assert::IsTrue(processor.GetFlag(FLAG_OVERFLOW));  // Bit 6 set
			Assert::IsFalse(processor.GetFlag(FLAG_ZERO));     // A & M != 0
		}
		TEST_METHOD(TestBITZeroPageZeroResult)
		{
			uint8_t rom[] = { BIT_ZEROPAGE, 0x10 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x0010, 0b00001111); // Lower nibble set
			processor.SetA(0b11110000); // A upper nibble set, so A & M = 0
			processor.Clock();
			Assert::IsFalse(processor.GetFlag(FLAG_NEGATIVE)); // Bit 7 clear
			Assert::IsFalse(processor.GetFlag(FLAG_OVERFLOW));  // Bit 6 clear
			Assert::IsTrue(processor.GetFlag(FLAG_ZERO));       // A & M == 0
		}
		TEST_METHOD(TestBITAbsoluteZeroResult)
		{
			uint8_t rom[] = { BIT_ABSOLUTE, 0x10, 0x15 };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0x1510, 0b00001111); // Lower nibble set
			processor.SetA(0b11110000); // A upper nibble set, so A & M = 0
			processor.Clock();
			Assert::IsFalse(processor.GetFlag(FLAG_NEGATIVE)); // Bit 7 clear
			Assert::IsFalse(processor.GetFlag(FLAG_OVERFLOW));  // Bit 6 clear
			Assert::IsTrue(processor.GetFlag(FLAG_ZERO));       // A & M == 0
		}
		TEST_METHOD(TestBMIRelative)
		{
			uint8_t rom[] = { BMI_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(true, FLAG_NEGATIVE); // Set negative to take branch
			processor.Clock();
			// After clocking BMI, PC should be at 0x8007 (start at 0x8000 + 2 for instruction + 5 for branch)
			Assert::AreEqual((uint16_t)0x8007, processor.GetPC());
		}
		TEST_METHOD(TestBMIRelativeNotTaken)
		{
			uint8_t rom[] = { BMI_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(false, FLAG_NEGATIVE); // Clear negative to not take branch
			processor.Clock();
			// After clocking BMI, PC should be at 0x8002 (start at 0x8000 + 2 for instruction)
			Assert::AreEqual((uint16_t)0x8002, processor.GetPC());
		}
		TEST_METHOD(TestBNERelative)
		{
			uint8_t rom[] = { BNE_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(false, FLAG_ZERO); // Clear zero to take branch
			processor.Clock();
			// After clocking BNE, PC should be at 0x8007 (start at 0x8000 + 2 for instruction + 5 for branch)
			Assert::AreEqual((uint16_t)0x8007, processor.GetPC());
		}
		TEST_METHOD(TestBNERelativeNotTaken)
		{
			uint8_t rom[] = { BNE_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(true, FLAG_ZERO); // Set zero to not take branch
			processor.Clock();
			// After clocking BNE, PC should be at 0x8002 (start at 0x8000 + 2 for instruction)
			Assert::AreEqual((uint16_t)0x8002, processor.GetPC());
		}
		TEST_METHOD(TestBPLRelative)
		{
			uint8_t rom[] = { BPL_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(false, FLAG_NEGATIVE); // Clear zero to take branch
			processor.Clock();
			// After clocking BNE, PC should be at 0x8007 (start at 0x8000 + 2 for instruction + 5 for branch)
			Assert::AreEqual((uint16_t)0x8007, processor.GetPC());
		}
		TEST_METHOD(TestBPLRelativeNotTaken)
		{
			uint8_t rom[] = { BPL_RELATIVE, 0x05, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			processor.SetFlag(true, FLAG_NEGATIVE); // Set zero to not take branch
			processor.Clock();
			// After clocking BNE, PC should be at 0x8002 (start at 0x8000 + 2 for instruction)
			Assert::AreEqual((uint16_t)0x8002, processor.GetPC());
		}
		TEST_METHOD(TestBRKImplied)
		{
			uint8_t rom[] = { BRK_IMPLIED, NOP_IMPLIED, NOP_IMPLIED, NOP_IMPLIED };
			cart.SetPRGRom(rom, sizeof(rom));
			bus.write(0xFFFE, 0x88); // Set IRQ vector low byte to 0x0088
			bus.write(0xFFFF, 0x80); // Set IRQ vector high byte to 0x8000
			processor.SetPC(0x8000);
			processor.Clock();
			// After clocking BRK, PC should be at the IRQ vector address stored at 0xFFFE/F
			uint16_t lo = bus.read(0xFFFE);
			uint16_t hi = bus.read(0xFFFF);
			uint16_t irqVector = (hi << 8) | lo;
			Assert::AreEqual(irqVector, processor.GetPC());
			// Also check that the Break flag is set
			Assert::IsTrue(processor.GetFlag(FLAG_BREAK));
		}
	};
}