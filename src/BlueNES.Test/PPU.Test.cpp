#include <cstdlib>
#include "pch.h"
#include "CppUnitTest.h"
#include "CPU.h"
#include "Cartridge.h"
#include "Bus.h"
#include "nes_ppu.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PPUTest
{
	TEST_CLASS(PPUTest)
	{
	private:
		Processor_6502 processor;
		Bus bus;
		Cartridge cart;
		NesPPU ppu;

	public:
		TEST_METHOD_INITIALIZE(TestSetup)
		{
			bus.cart = &cart;
			bus.cpu = &processor;
			processor.bus = &bus;
			ppu.bus = &bus;
			processor.Initialize();
			processor.Activate(true);
		}

		TEST_METHOD(TestReadWritePPUCTRL)
		{
			ppu.write_register(0x2000, 0x80);
			Assert::AreEqual((uint8_t)0x80, ppu.read_register(0x2000));
		}

		TEST_METHOD(TestWritePPUADDR)
		{
			ppu.write_register(PPUADDR, 0x20);
			// The first write sets the high byte of the TEMP address
			// But does not update the actual VRAM address yet
			Assert::AreEqual((uint16_t)0x0000, ppu.GetVRAMAddress());
			ppu.write_register(PPUADDR, 0x00);
			// The second write sets the low byte and updates the VRAM address
			Assert::AreEqual((uint16_t)0x2000, ppu.GetVRAMAddress());
		}
		TEST_METHOD(TestWritePPUSCROLL)
		{
			ppu.write_register(PPUSCROLL, 0x05);
			ppu.write_register(PPUSCROLL, 0x06);
			auto [x, y] = ppu.GetPPUScroll();
			Assert::AreEqual((uint8_t)0x05, x);
			Assert::AreEqual((uint8_t)0x06, y);
		}
		TEST_METHOD(TestWritePPUDATA)
		{
			ppu.SetVRAMAddress(0x2000);
			ppu.write_register(PPUDATA, 0x55);
			Assert::AreEqual((uint16_t) 0x2001, ppu.GetVRAMAddress());
			Assert::AreEqual((uint8_t)0x55, ppu.ReadVRAM(0x2000));
		}
		TEST_METHOD(TestWritePPUDATAVert)
		{
			ppu.write_register(PPUCTRL, 0x04); // Set vertical increment
			ppu.SetVRAMAddress(0x2000);
			ppu.write_register(PPUDATA, 0x55);
			Assert::AreEqual((uint16_t)0x2020, ppu.GetVRAMAddress());
			Assert::AreEqual((uint8_t)0x55, ppu.ReadVRAM(0x2000));
		}
	};
}