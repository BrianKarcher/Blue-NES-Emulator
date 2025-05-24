#include <cstdlib>
#include "pch.h"
#include "CppUnitTest.h"
#include "processor_6502.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace BlueNESTest
{
	TEST_CLASS(BlueNESTest)
	{
	public:
		
		TEST_METHOD(TestADCImmediate1)
		{
			Processor_6502 processor;
			uint8_t data[] = { 0x69, 0x20 };
			uint8_t memory[2048];
			processor.Initialize(data, memory);
			processor.RunStep();
			Assert::AreEqual((uint8_t)0x20, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCImmediateWithCarry)
		{
			Processor_6502 processor;
			uint8_t data[] = { 0x69, 0x20 };
			uint8_t memory[2048];
			processor.Initialize(data, memory);
			processor.SetA(0x11);
			processor.SetFlag(true, FLAG_CARRY);
			processor.RunStep();
			// 0x11 + 0x20 + 1 (Carry) = 0x32
			Assert::AreEqual((uint8_t)0x32, processor.GetA());
			Assert::IsFalse(processor.GetFlag(FLAG_CARRY));
		}

		TEST_METHOD(TestADCImmediateWithOverflow)
		{
			Processor_6502 processor;
			// $ef + $20 will result in overflow
			processor.SetA(0xef);
			uint8_t data[] = { 0x69, 0x20 };
			uint8_t memory[2048];
			processor.Initialize(data, memory);
			processor.RunStep();
			Assert::AreEqual((uint8_t)0xf, processor.GetA());
			Assert::IsTrue(processor.GetFlag(FLAG_CARRY));
		}
	};
}
