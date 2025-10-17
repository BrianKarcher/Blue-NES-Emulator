#include "nes_ppu.h"

NesPPU::NesPPU()
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
				vramAddr = (vramAddr & 0x00FF) | (value & 0x3F) << 8; // First write (high byte)
			}
			else
			{
				vramAddr = (vramAddr & 0xFF00) | value; // Second write (low byte)
			}
			writeToggle = !writeToggle;
			break;
		case 0x07: // PPUDATA
			// Handle PPUDATA write here
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
	// Render a full frame to the back buffer here

}