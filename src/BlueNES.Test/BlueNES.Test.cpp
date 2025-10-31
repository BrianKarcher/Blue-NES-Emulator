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
	};
}