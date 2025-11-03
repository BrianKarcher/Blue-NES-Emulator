#include "ScrollTest.h"
#include "nes_ppu.h"

void ScrollTest::Setup()
{
	// Initialize test environment
	m_ppuCtrl = 0x00;
	m_scrollY = 0;
}

void ScrollTest::Update()
{
	m_scrollY++;
	if (m_scrollY >= 240) {
		m_scrollY -= 240;
		m_ppuCtrl ^= 0x02; // Switch nametable
		ppu.write_register(PPUCTRL, m_ppuCtrl); // Update PPUCTRL with current nametable
	}

	ppu.write_register(PPUSCROLL, 0x00); // No horizontal scroll
	ppu.write_register(PPUSCROLL, m_scrollY);
	//scrollX++;

	// Move sprite
	oam[3] += 1; // X position
	oam[7] += 1; // X position of 2nd sprite
	oam[11] += 1; // X position of 3rd sprite
	oam[15] += 1; // X position of 4th sprite
	PerformDMA();
}