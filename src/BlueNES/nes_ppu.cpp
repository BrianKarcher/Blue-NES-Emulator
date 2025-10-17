#include "nes_ppu.h"

NesPPU::NesPPU() :
	m_vram()
{
	m_backBuffer.fill(0xFFFF00FF); // Initialize to opaque black
}

NesPPU::~NesPPU()
{

}

void NesPPU::set_chr_rom(uint8_t* chrData, size_t size)
{
	m_pchrRomData = chrData;
	m_chrRomSize = size;
}

void NesPPU::reset()
{
	// Reset PPU state here
	m_backBuffer.fill(0xFFFF0000); // Clear back buffer to opaque black
}

void NesPPU::step()
{
	// Emulate one PPU cycle here

}

void NesPPU::write_vram(uint16_t addr, uint8_t value)
{
	addr &= 0x3FFF; // Mask to 14 bits
	if (addr < 0x2000) {
		// Write to CHR-RAM (if enabled)
		// TODO: Support CHR-RAM vs CHR-ROM distinction
		if (m_pchrRomData && addr < m_chrRomSize) {
			m_pchrRomData[addr] = value;
		}
		// Else ignore write (CHR-ROM is typically read-only)
		return;
	} else if (addr < 0x3F00) {
		// Name tables and attribute tables
		m_vram[addr] = value;
		return;
	} else if (addr < 0x4000) {
		// Palette RAM (mirrored every 32 bytes)
		uint8_t paletteAddr = addr & 0x1F;
		// TODO: Add support for the universal background color mirroring
		m_vram[0x3F00 + paletteAddr] = value;
		return;
	}
}

void NesPPU::write_register(uint16_t addr, uint8_t value)
{
	// addr is in the range 0x2000 to 0x2007
	// It is the CPU that writes to these registers
	// addr is mirrored every 8 bytes up to 0x3FFF so we mask it
	switch (addr & 0x07) {
		case 0x00: // PPUCTRL
		// Handle PPUCTRL write here
			break;
		case 0x01: // PPUMASK
			// Handle PPUMASK write here
			break;
		case 0x02: // PPUSTATUS (read-only)
			// Ignore writes to PPUSTATUS
			break;
		case 0x03: // OAMADDR
			// Handle OAMADDR write here
			break;
		case 0x04: // OAMDATA
			// Handle OAMDATA write here
			break;
		case 0x05: // PPUSCROLL
			// Handle PPUSCROLL write here
			break;
		case 0x06: // PPUADDR
			if (!writeToggle)
			{
				// The PPU address space is 14 bits (0x0000 to 0x3FFF), so we mask accordingly
				// TODO: Test this to make sure uint16_t cast works as expected
				vramAddr = (vramAddr & 0x00FF) | (value & 0x3F) << 8; // First write (high byte)
			}
			else
			{
				vramAddr = (vramAddr & 0xFF00) | value; // Second write (low byte)
			}
			// If the program doesn't do two writes in a row, the behavior is undefined.
			// It's their fault if their code is broken.
			writeToggle = !writeToggle;
			break;
		case 0x07: // PPUDATA
			write_vram(vramAddr, value);
			// Increment VRAM address based on PPUCTRL setting (TODO: not implemented yet, default to 1)
			vramAddr += 1;
			break;
	}
}

uint8_t NesPPU::read_register(uint16_t addr)
{
	// Handle reads from PPU registers here
	return 0;
}

void NesPPU::render_scanline()
{
	// Render a single scanline to the back buffer here
}

void NesPPU::render_frame()
{
	//m_backBuffer.fill(0xFF000000); // For testing, fill with opaque black
	for (int r = 0; r < 240; r++) {
		for (int c = 0; c < 256; c++) {
			// Simple test pattern: color based on position
			uint8_t newr = r / 30;
			uint8_t newc = c / 32;
			// Simple test pattern: color based on nametable position
			uint8_t colorIndex = m_vram[0x2000 + newr * 32 + newc]; // Cycle through palette
			m_backBuffer[r * 256 + c] = m_nesPalette[colorIndex];

			//uint8_t colorIndex = (r / 15 + c / 16) % 64; // Cycle through palette
			//m_backBuffer[r * 256 + c] = m_nesPalette[colorIndex];
			//m_backBuffer[r * 256 + c] = m_nesPalette[(r + c) % 64];
		}
	}
}