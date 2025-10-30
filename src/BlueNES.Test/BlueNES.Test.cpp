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
	public:
		
		TEST_METHOD(TestADCImmediate1)
		{
			Processor_6502 processor;
			uint8_t data[] = { ADC_IMMEDIATE, 0x20 };
			//uint8_t memory[2048];
			Bus bus;
			Cartridge cart;
			cart.SetPRGRom(data, sizeof(data));

			bus.cart = &cart;
			bus.cpu = &processor;
			processor.bus = &bus;
			processor.Initialize();
			processor.Clock();
			Assert::AreEqual((uint8_t)0x20, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		//TEST_METHOD(TestADCImmediateWithCarry)
		//{
		//	Processor_6502 processor;
		//	uint8_t data[] = { ADC_IMMEDIATE, 0x20 };
		//	uint8_t memory[2048];
		//	processor.Initialize(data, memory);
		//	processor.SetA(0x11);
		//	processor.SetFlag(true, FLAG_CARRY);
		//	processor.RunStep();
		//	// 0x11 + 0x20 + 1 (Carry) = 0x32
		//	Assert::AreEqual((uint8_t)0x32, processor.GetA());
		//	Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		//}

		//TEST_METHOD(TestADCImmediateWithOverflow)
		//{
		//	Processor_6502 processor;
		//	// $ef + $20 will result in overflow
		//	processor.SetA(0xef);
		//	uint8_t data[] = { ADC_IMMEDIATE, 0x20 };
		//	uint8_t memory[2048];
		//	processor.Initialize(data, memory);
		//	processor.RunStep();
		//	Assert::AreEqual((uint8_t)0xf, processor.GetA());
		//	Assert::IsTrue(processor.GetFlag(FLAG_CARRY));
		//}

		//TEST_METHOD(TestADCZeroPage)
		//{
		//	Processor_6502 processor;
		//	// Add what is at zero page 0x15 to A.
		//	uint8_t data[] = { ADC_ZEROPAGE, 0x15 };
		//	uint8_t memory[2048];
		//	memory[0x15] = 0x69;
		//	processor.Initialize(data, memory);
		//	processor.SetA(0x18);
		//	processor.RunStep();
		//	Assert::AreEqual((uint8_t)0x81, processor.GetA());
		//	Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		//}

		//TEST_METHOD(TestADCZeroPage_X)
		//{
		//	Processor_6502 processor;
		//	// Add what is at zero page 0x15 to A.
		//	uint8_t data[] = { ADC_ZEROPAGE_X, 0x15 };
		//	uint8_t memory[2048];
		//	memory[0x16] = 0x69;
		//	processor.Initialize(data, memory);
		//	processor.SetA(0x18);
		//	processor.SetX(0x1);
		//	processor.RunStep();
		//	Assert::AreEqual((uint8_t)0x81, processor.GetA());
		//	Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		//}

		//TEST_METHOD(TestADCAbsolute)
		//{
		//	Processor_6502 processor;
		//	// I think the 6502 stores in little endian, need to double check
		//	uint8_t data[] = { ADC_ABSOLUTE, 0x23, 0x3 };
		//	uint8_t memory[2048];
		//	memory[0x323] = 0x40;
		//	processor.Initialize(data, memory);
		//	processor.SetA(0x20);
		//	processor.RunStep();
		//	Assert::AreEqual((uint8_t)0x60, processor.GetA());
		//	Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		//}
	};
}
