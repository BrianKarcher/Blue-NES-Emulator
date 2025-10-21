#include "nes_ppu.h"
#include <string>
#include <cstdint>
#include <WinUser.h>

HWND m_hwnd;

NesPPU::NesPPU()
{
	m_backBuffer.fill(0xFF000000); // Initialize to opaque black
	oam.fill(0xFF);
	m_scrollX = 0;
	m_scrollY = 0;
	m_ppuCtrl = 0;
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

// Map a PPU address ($2000–$2FFF) to actual VRAM offset (0–0x7FF)
uint16_t NesPPU::MirrorAddress(uint16_t addr)
{
	uint8_t mirrorMode = MirrorMode::VERTICAL; // m_ppuCtrl & 0x03; // Bits 0-1 of PPUCTRL determine nametable
	addr = (addr - 0x2000) & 0x0FFF; // Normalize into 0x000–0xFFF (4KB range)
	uint16_t table = addr / 0x400;   // Which of the 4 logical nametables
	uint16_t offset = addr % 0x400;  // Offset within that table

	switch (mirrorMode)
	{
	case MirrorMode::VERTICAL:
		// NT0 and NT2 -> physical 0
		// NT1 and NT3 -> physical 1
		// pattern: 0,1,0,1
		return (table % 2) * 0x400 + offset;

	case MirrorMode::HORIZONTAL:
		// NT0 and NT1 -> physical 0
		// NT2 and NT3 -> physical 1
		// pattern: 0,0,1,1
		return ((table / 2) * 0x400) + offset;

	case MirrorMode::SINGLE_SCREEN:
		// All nametables map to 0x000
		return 0x000 + offset;

	//case MirrorMode::SingleScreenUpper:
	//	// All nametables map to 0x400
	//	return 0x400 + offset;

	case MirrorMode::FOUR_SCREEN:
		// Cartridge provides 4KB VRAM, so direct mapping
		return addr; // No mirroring

	default:
		return 0; // Safety
	}
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
	switch (addr) {
		case PPUCTRL: // PPUCTRL
		// Handle PPUCTRL write here
			break;
		case PPUMASK: // PPUMASK
			// Handle PPUMASK write here
			break;
		case PPUSTATUS: // PPUSTATUS (read-only)
			// Ignore writes to PPUSTATUS
			break;
		case OAMADDR: // OAMADDR
			oamAddr = value;
			break;
		case OAMDATA: // OAMDATA
			oam[oamAddr++] = value;
			break;
		case PPUSCROLL: // PPUSCROLL
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
		case PPUADDR: // PPUADDR
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
		case PPUDATA: // PPUDATA
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

void NesPPU::render_frame()
{
	// Clear back buffer
	// m_backBuffer.fill(0xFF000000);
	int scrollX = m_scrollX & 0xFF; // Fine X scrolling (0-255)
	int scrollY = m_scrollY & 0xFF; // Fine Y scrolling (0-239)
	std::array<Sprite, 8> secondaryOAM{};
	for (int i = 0; i < 8; ++i) {
		secondaryOAM[i] = { 0xFF, 0xFF, 0xFF, 0xFF }; // Initialize to empty sprite
	}
	uint8_t baseNametableIndex = (m_ppuCtrl & 0x03); // Bits 0-1 of PPUCTRL determine nametable base
	uint16_t baseNametableAddr = 0x2000 + (baseNametableIndex * 0x400);
	// Render the 256x240 visible area
	for (int screenY = 0; screenY < 240; ++screenY)
	{
		// uint16_t currentNametableIndex = baseNametableAddr;
		int fineY = (screenY + scrollY) % (NAMETABLE_HEIGHT * TILE_SIZE); // Wrap around vertically
		//bool flipped = false;
		for (int screenX = 0; screenX < 256; ++screenX)
		{
			int fineX = (scrollX + screenX) % (NAMETABLE_WIDTH * TILE_SIZE);
			// Compute the base nametable index based on coarse scroll
			uint16_t coarseX = (scrollX + screenX) / (NAMETABLE_WIDTH * TILE_SIZE);
			uint16_t coarseY = (scrollY + screenY) / (NAMETABLE_HEIGHT * TILE_SIZE);

			// Determine which nametable we’re in (0–3)
			uint8_t nametableSelect = (m_ppuCtrl & 0x03);
			if (coarseX % 2) nametableSelect ^= 1;       // Switch horizontally
			if (coarseY % 2) nametableSelect ^= 2;       // Switch vertically

			uint16_t nametableAddr = 0x2000 + nametableSelect * 0x400;

			// Convert to tile coordinates
			int tileCol = fineX / TILE_SIZE;
			int tileRow = fineY / TILE_SIZE;
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
			int bgPixelInTileX = fineX % TILE_SIZE;
			int bgPixelInTileY = fineY % TILE_SIZE;
			uint8_t bgColorIndex = get_tile_pixel_color_index(tileIndex, bgPixelInTileX, bgPixelInTileY);
			// Handle both background and sprite color mapping here since we have to deal with
			// transparency and priority
			// Added bonus: Single draw call to set pixel in back buffer
			uint32_t bgColor = 0;
			if (bgColorIndex == 0) {
				bgColor = m_nesPalette[m_vram[0x3F00]]; // Transparent color (background color)
			}
			else {
				bgColor = palette[bgColorIndex]; // Map to actual color from palette
			}
			// Set pixel in back buffer
			m_backBuffer[(screenY * 256) + screenX] = bgColor;
			bool foundSprite = false;
			for (int i = 0; i < 8 && !foundSprite; ++i) {
				Sprite& sprite = secondaryOAM[i];
				if (sprite.y >= 0xF0) {
					continue; // Empty sprite slot
				}
				int spriteY = sprite.y + 1; // Adjust for off-by-one
				int spriteX = sprite.x;
				if (screenX >= spriteX && screenX < (spriteX + 8)) {
					// Sprite pixel coordinates
					int spritePixelX = screenX - spriteX;
					int spritePixelY = screenY - spriteY;

					bool flipHorizontal = sprite.attributes & 0x40;
					bool flipVertical = sprite.attributes & 0x80;
					if (flipHorizontal) {
						spritePixelX = 7 - spritePixelX;
					}
					if (flipVertical) {
						spritePixelY = 7 - spritePixelY;
					}
					// Get sprite palette
					uint8_t spritePaletteIndex = sprite.attributes & 0x03;
					std::array<uint16_t, 4> spritePalette;
					get_palette(spritePaletteIndex + 4, spritePalette); // Sprite palettes start at 0x3F10
					// Get color index from sprite tile
					uint8_t spriteColorIndex = get_tile_pixel_color_index(sprite.tileIndex, spritePixelX, spritePixelY);
					if (spriteColorIndex != 0) { // Non-transparent pixel
						uint16_t spriteColor = spritePalette[spriteColorIndex];
						// Handle priority (not implemented yet, assuming sprites are always on top)
						m_backBuffer[(screenY * 256) + screenX] = spriteColor;
						foundSprite = true;
					}
				}
			}
		}
		// Sprite evaluation for this scanline get rendered on the next scanline
		// All sprites on the NES have an off-by-one error for the Y position
		EvaluateSprites(screenY, secondaryOAM);
	}
}

void NesPPU::EvaluateSprites(int screenY, std::array<Sprite, 8>& newOam)
{
	for (int i = 0; i < 8; ++i) {
		newOam[i] = { 0xFF, 0xFF, 0xFF, 0xFF }; // Initialize to empty sprite
	}
	// Evaluate sprites for this scanline
	int spriteCount = 0;
	for (int i = 0; i < 64; ++i) {
		int spriteY = oam[i * 4]; // Y position of the sprite
		if (spriteY > 0xF0) {
			continue; // Empty sprite slot
		}
		int spriteHeight = 8; // For now, assume 8x8 sprites. TODO: Support 8x16 sprites.
		if (screenY >= spriteY && screenY < (spriteY + spriteHeight)) {
			if (spriteCount < 8) {
				// Copy sprite data to new OAM
				newOam[spriteCount].y = oam[i * 4];
				newOam[spriteCount].tileIndex = oam[i * 4 + 1];
				newOam[spriteCount].attributes = oam[i * 4 + 2];
				newOam[spriteCount].x = oam[i * 4 + 3];
				spriteCount++;
			}
			else {
				// Sprite overflow - more than 8 sprites on this scanline
				// Set sprite overflow flag in PPUSTATUS (not implemented yet)
				break;
			}
		}
	}
}

uint8_t NesPPU::get_tile_pixel_color_index(uint8_t tileIndex, uint8_t pixelInTileX, uint8_t pixelInTileY)
{
	int tileBase = tileIndex * 16; // 16 bytes per tile

	uint8_t byte1 = m_pchrRomData[tileBase + pixelInTileY];     // bitplane 0
	uint8_t byte2 = m_pchrRomData[tileBase + pixelInTileY + 8]; // bitplane 1

	uint8_t bit0 = (byte1 >> (7 - pixelInTileX)) & 1;
	uint8_t bit1 = (byte2 >> (7 - pixelInTileX)) & 1;
	uint8_t colorIndex = (bit1 << 1) | bit0;

	return colorIndex;
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