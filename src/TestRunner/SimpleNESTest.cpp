#include "SimpleNESTest.h"
#include "nes_ppu.h"
#include "Core.h"
#include "Bus.h"
#include <iostream>
#include "IntegrationRunner.h"
#include <Windows.h>

void SimpleNESTest::Setup(IntegrationRunner& runner)
{
    m_runner = &runner;
    m_core = runner.GetCore();
    //oam = m_runner->oam.data();
    // Initialize test environment
    bus = &m_core->bus;
    ppu = &m_core->ppu;

    size_t bytesRead;
    //const char* testFile = "simple.nes";
    //const char* testFile = "sprite.nes";
    //const char* testFile = "hello_world.nes";
	//const char* testFile = "roms\\dual_pattern_tables.nes";
    //const char* testFile = "roms\\hor_scroll.nes";
    //const char* testFile = "roms\\vert_scroll.nes";
    //const char* testFile = "roms\\hor_scroll_full.nes";
    //const char* testFile = "roms\\Super Mario Bros.nes";
    //const char* testFile = "roms\\snake.nes";
    const char* testFile = "roms\\vert_nt_draw.nes";
    
    //uint8_t* buffer = m_runner->LoadFile("test-chr-rom.chr", bytesRead);
	m_runner->GetCore()->cart.LoadROM(testFile);

    bus->cpu->Activate(true);
    //bus->cart->SetCHRRom(buffer, bytesRead);
    //delete[] buffer;
}

/// <summary>
/// This test does nothing outside of running the VERY small NES program loaded in Setup.
/// The program tests basic PPU functionality and writes results to $0000 in RAM.
/// </summary>
void SimpleNESTest::Update()
{
    // Create a 2-character wide string: [char, null terminator]
 //   wchar_t text[2];
 //   uint8_t val = bus->read(0x0001);
 //   text[0] = static_cast<wchar_t>(val);
 //   text[1] = L'\0';
	//SetWindowText(m_runner->GetWindowHandle(), text);
}