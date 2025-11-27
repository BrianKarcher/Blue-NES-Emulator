#include <cstdlib>
#include "pch.h"
#include "CppUnitTest.h"
#include "CPU.h"
#include "Cartridge.h"
#include "Bus.h"
#include "nes_ppu.h"
#include "RendererLoopy.h"
#include "Core.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

/* 
	Tests centered around the Loopy renderer. Trust me, we need these. 
*/
namespace PPUTest
{
	TEST_CLASS(LoopyTest)
	{
	private:
		//Processor_6502 processor;
		//Bus bus;
		//Cartridge cart;
		//NesPPU ppu;
		Core core;
		RendererLoopy loopyRenderer;

	public:
		TEST_METHOD_INITIALIZE(TestSetup)
		{
			core.Initialize();
			ines_file_t inesLoader;
			core.bus.cart->SetMapper(0, inesLoader);
			//core.cpu.PowerOn();
			//core.cpu.Activate(true);
			//core.cpu.SetPC(0x8000);
			loopyRenderer.initialize(&core.ppu);
		}
	};
}