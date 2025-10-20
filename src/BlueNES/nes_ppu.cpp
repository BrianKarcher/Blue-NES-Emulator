#include "nes_ppu.h"
#include <string>
#include <cstdint>
#include <WinUser.h>

HWND m_hwnd;

NesPPU::NesPPU():
	m_vram()
{
	m_backBuffer.fill(0xFF000000); // Initialize to opaque black
	oam.fill(0xFF);
	m_scrollX = 0;
	m_scrollY = 0;
}

NesPPU::~NesPPU()
{

}

void NesPPU::set_hwnd(HWND hwnd) {
	m_hwnd = hwnd;
}

void NesPPU::set_chr_rom(uint8_t* chrData, size_t size)
{
	m_pchrRomData = chrData;
	m_chrRomSize = size;
}

void NesPPU::SetMirrorMode(MirrorMode mode) {
	mirrorMode = mode;
}

// Convert nametable address to physical VRAM address
uint16_t NesPPU::MirrorAddress(uint16_t addr) {
	addr &= 0x2FFF; // Mirror to nametable range
	uint16_t offset = addr & 0x3FF;
	uint16_t table = (addr >> 10) & 0x03;

	switch (mirrorMode) {
	case HORIZONTAL:
		return 0x2000 + ((table & 0x02) << 10) + offset;
	case VERTICAL:
		return 0x2000 + ((table & 0x01) << 10) + offset;
	case SINGLE_SCREEN_LOW:
		return 0x2000 + offset;
	case SINGLE_SCREEN_HIGH:
		return 0x2400 + offset;
	case FOUR_SCREEN:
		return 0x2000 + (table << 10) + offset;
	}
	return 0x2000 + offset;
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
		if (paletteAddr % 4 == 0) {
			// Handle mirroring of the background color by setting the address to 0x3F00
			paletteAddr = 0;
		}
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
			oamAddr = value;
			break;
		case 0x04: // OAMDATA
			oam[oamAddr++] = value;
			break;
		case 0x05: // PPUSCROLL
			// Handle PPUSCROLL write here
			if (!writeToggle)
			{
				m_scrollX = value; // First write sets horizontal scroll
			}
			else
			{
				m_scrollY = value; // Second write sets vertical scroll
			}
			// Note that writeToggle is shared with PPUADDR
			// I retain to mimic hardware behavior
			writeToggle = !writeToggle;
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

// OAM DMA - Direct Memory Access for sprites
void NesPPU::OAMDMA(uint8_t* cpuMemory, uint16_t page) {
	uint16_t addr = page << 8;
	for (int i = 0; i < 256; i++) {
		oam[oamAddr++] = cpuMemory[addr + i];
	}
	// OAM DMA takes 513 or 514 CPU cycles depending on odd/even alignment
	// OAMADDR wraps around automatically in hardware
	oamAddr &= 0xFF;
}

// Sprite data for current scanline
struct Sprite {
	uint8_t x;
	uint8_t y;
	uint8_t tile;
	uint8_t attr;
	bool isSprite0;
};
std::array<Sprite, 8> secondaryOAM;

void NesPPU::render_frame()
{
	// Clear back buffer
	// m_backBuffer.fill(0xFF000000);

	// Nametable starts at 0x2000 in VRAM
	// TODO : Support multiple nametables and mirroring
	const uint16_t nametableAddr = 0x2000;
	int scrollX = m_scrollX & 0xFF; // Fine X scrolling (0-255)
	int scrollY = m_scrollY & 0xFF; // Fine Y scrolling (0-239)

	// Render the 256x240 visible area
	for (int screenY = 0; screenY < 240; ++screenY)
	{
		for (int screenX = 0; screenX < 256; ++screenX)
		{
			// Compute world coordinates with scrolling
			int worldX = (screenX + scrollX) % (NAMETABLE_WIDTH * TILE_SIZE); // Wrap around horizontally
			int worldY = (screenY + scrollY) % (NAMETABLE_HEIGHT * TILE_SIZE); // Wrap around vertically
			// Convert to tile coordinates
			int tileCol = worldX / TILE_SIZE;
			int tileRow = worldY / TILE_SIZE;
			int tileIndex = m_vram[nametableAddr + tileRow * NAMETABLE_WIDTH + tileCol];

			// Get attribute byte for the tile
			int attrRow = tileRow / 4;
			int attrCol = tileCol / 4;
			uint8_t attributeByte = m_vram[(nametableAddr | 0x3c0) + attrRow * 8 + attrCol];

			uint8_t paletteIndex = 0;
			get_palette_index_from_attribute(attributeByte, tileRow, tileCol, paletteIndex);

			std::array<uint16_t, 4> palette;
			get_palette(paletteIndex, palette); // For now we don't use the colors

			// Get the color of the specific pixel within the tile
			int pixelInTileX = worldX % TILE_SIZE;
			int pixelInTileY = worldY % TILE_SIZE;
			uint32_t color = get_tile_pixel_color(tileIndex, pixelInTileX, pixelInTileY, palette);
			// Set pixel in back buffer
			m_backBuffer[(screenY * 256) + screenX] = color;
		}
	}
}

uint32_t NesPPU::get_tile_pixel_color(uint8_t tileIndex, uint8_t pixelInTileX, uint8_t pixelInTileY, std::array<uint16_t, 4>& palette)
{
	int tileBase = tileIndex * 16; // 16 bytes per tile

	uint8_t byte1 = m_pchrRomData[tileBase + pixelInTileY];     // bitplane 0
	uint8_t byte2 = m_pchrRomData[tileBase + pixelInTileY + 8]; // bitplane 1

	uint8_t bit0 = (byte1 >> (7 - pixelInTileX)) & 1;
	uint8_t bit1 = (byte2 >> (7 - pixelInTileX)) & 1;
	uint8_t colorIndex = (bit1 << 1) | bit0;

	if (colorIndex == 0) {
		return m_nesPalette[m_vram[0x3F00]]; // Transparent color (background color)
	}
	else {
		return palette[colorIndex]; // Map to actual color from palette
	}
}

void NesPPU::render_nametable()
{
    // Clear back buffer
    // m_backBuffer.fill(0xFF000000);
    
    // Nametable starts at 0x2000 in VRAM
	// TODO : Support multiple nametables and mirroring
    const uint16_t nametableAddr = 0x2000;
    
	// TODO: For now, we just render the nametable directly without scrolling or attribute tables
	std::array<uint16_t, 4> palette;
	// Render the 32x30 tile nametable
    for (int row = 0; row < 30; row++) {
        for (int col = 0; col < 32; col++) {
            // Get the tile index from the nametable in VRAM
            uint8_t tileIndex = m_vram[nametableAddr + row * 32 + col];
            
            // Calculate pixel coordinates
            int pixelX = col * 8;  // Each tile is 8x8 pixels
            int pixelY = row * 8;

			int attrRow = row / 4;
			int attrCol = col / 4;
			// Get attribute byte for the tile
			uint8_t attributeByte = m_vram[0x23c0 + attrRow * 8 + attrCol];

			uint8_t paletteIndex = 0;
			get_palette_index_from_attribute(attributeByte, row, col, paletteIndex);
			get_palette(paletteIndex, palette); // For now we don't use the colors
            // Render the tile at the calculated position
            render_tile(pixelY, pixelX, tileIndex, palette);
        }
    }
}

void NesPPU::get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex)
{
	// Each attribute byte covers a 4x4 tile area (32x32 pixels)
	// Determine which quadrant of the attribute byte the tile is in
	int quadrantRow = (tileRow % 4) / 2; // 0 or 1
	int quadrantCol = (tileCol % 4) / 2; // 0 or 1
	// Extract the corresponding 2 bits for the palette index
	switch (quadrantRow) {
		case 0:
			switch (quadrantCol) {
				case 0:
					paletteIndex = attributeByte & 0x03; // Bits 0-1
					break;
				case 1:
					paletteIndex = (attributeByte >> 2) & 0x03; // Bits 2-3
					break;
			}
			break;
		case 1:
			switch (quadrantCol) {
				case 0:
					paletteIndex = (attributeByte >> 4) & 0x03; // Bits 4-5
					break;
				case 1:
					paletteIndex = (attributeByte >> 6) & 0x03; // Bits 6-7
					break;
			}
			break;
	}
}

void NesPPU::get_palette(uint8_t paletteIndex, std::array<uint16_t, 4>& colors)
{
	// Each palette consists of 4 colors, starting from 0x3F00 in VRAM
	uint16_t paletteAddr = 0x3F00 + (paletteIndex * 4);
	colors[0] = m_nesPalette[m_vram[paletteAddr] & 0x3F];
	colors[1] = m_nesPalette[m_vram[paletteAddr + 1] & 0x3F];
	colors[2] = m_nesPalette[m_vram[paletteAddr + 2] & 0x3F];
	colors[3] = m_nesPalette[m_vram[paletteAddr + 3] & 0x3F];
}

void NesPPU::render_chr_rom()
{
    //m_backBuffer.fill(0xFF000000); // For testing, fill with opaque black
	std::array<uint16_t, 4> palette;
	// Draw a whole pattern table for testing
	for (int pr = 0; pr < 128; pr += 8) {
		for (int pc = 0; pc < 128; pc += 8) {
			int tileRow = pr / 8;
			int tileCol = pc / 8;
			int tileIndex = (tileRow * 16) + tileCol;
			render_tile(pr, pc, tileIndex, palette);
		}
	}
}

void NesPPU::render_tile(int pr, int pc, int tileIndex, std::array<uint16_t, 4>& colors) {
	int tileBase = tileIndex * 16; // 16 bytes per tile

	for (int y = 0; y < 8; y++) {
		uint8_t byte1 = m_pchrRomData[tileBase + y];     // bitplane 0
		uint8_t byte2 = m_pchrRomData[tileBase + y + 8]; // bitplane 1

		for (int x = 0; x < 8; x++) {
			uint8_t bit0 = (byte1 >> (7 - x)) & 1;
			uint8_t bit1 = (byte2 >> (7 - x)) & 1;
			uint8_t colorIndex = (bit1 << 1) | bit0;

			uint16_t actualColor = 0;
			if (colorIndex == 0) {
				actualColor = m_nesPalette[m_vram[0x3F00]]; // Transparent color (background color)
			}
			else {
				actualColor = colors[colorIndex]; // Map to actual color from palette
			}
			int renderX = pc + x;
			int renderY = pr + y;
			if (renderX < 0 || renderY < 0 || renderX >= 256 || renderY >= 240)
				continue;
			m_backBuffer[(renderY * 256) + renderX] = actualColor;
		}
	}
}