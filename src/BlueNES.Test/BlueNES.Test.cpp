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
		
		TEST_METHOD(TestMethod1)
		{
			Processor_6502* processor = new Processor_6502();
			uint8_t data[] = { 0x69 };
			processor->Initialize(&data[0]);
			Assert::AreEqual((uint8_t)0x0, processor->GetA());
		}
	};
}
