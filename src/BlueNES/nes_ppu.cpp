#include "nes_ppu.h"
#include <string>
#include <cstdint>
#include <WinUser.h>
#include "Bus.h"
#include "Core.h"

HWND m_hwnd;

NesPPU::NesPPU()
{
	m_backBuffer.fill(0xFF000000); // Initialize to opaque black
	oam.fill(0xFF);
	m_scrollX = 0;
	m_scrollY = 0;
	m_ppuCtrl = 0;
	oamAddr = 0;
}

NesPPU::~NesPPU()
{

}

void NesPPU::set_hwnd(HWND hwnd) {
	m_hwnd = hwnd;
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
	switch (addr) {
	case PPUCTRL:
		m_ppuCtrl = value;
		break;
	case PPUMASK: // PPUMASK
		m_ppuMask = value;
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
			tempVramAddr = (tempVramAddr & 0x00FF) | ((value & 0x3F) << 8); // First write (high byte)
			// vramAddr = (vramAddr & 0x00FF) | (value & 0x3F) << 8; // First write (high byte)
		}
		else
		{
			tempVramAddr = (tempVramAddr & 0x7F00) | value; // Second write (low byte)
			vramAddr = tempVramAddr; // Second write (low byte)
		}
		// If the program doesn't do two writes in a row, the behavior is undefined.
		// It's their fault if their code is broken.
		writeToggle = !writeToggle;
		break;
	case PPUDATA: // PPUDATA
		write_vram(vramAddr, value);
		// Increment VRAM address based on PPUCTRL setting (TODO: not implemented yet, default to 1)
		if (vramAddr >= 0x3F00) {
			// Palette data always increments by 1
			vramAddr += 1;
			return;
		}
		vramAddr += m_ppuCtrl & 0x04 ? 32 : 1;
		break;
	}
}

uint8_t NesPPU::read_register(uint16_t addr)
{
	switch (addr)
	{
	case PPUCTRL:
	{
		return m_ppuCtrl;
	}
	case PPUMASK:
	{
		// not typically readable, return 0
		return 0;
	}
	case PPUSTATUS:
	{
		writeToggle = false; // Reset write toggle on reading PPUSTATUS
		// Return PPU status register value and clear VBlank flag
		uint8_t status = m_ppuStatus;
		m_ppuStatus &= ~PPUSTATUS_VBLANK;
		return status;
	}
	case OAMADDR:
	{
		return oamAddr;
	}
	case OAMDATA:
	{
		// Return OAM data at current OAMADDR
		return oam[oamAddr];
	}
	case PPUSCROLL:
	{
		// PPUSCROLL is write-only, return 0
		return 0;
	}
	case PPUADDR:
	{
		// PPUADDR is write-only, return 0
		return 0;
	}
	case PPUDATA:
	{
		// Read from VRAM at current vramAddr
		uint8_t value = ReadVRAM(vramAddr);
		// Increment VRAM address based on PPUCTRL setting
		if (vramAddr >= 0x3F00) {
			// Palette data always increments by 1
			vramAddr += 1;
			return value;
		}
		vramAddr += m_ppuCtrl & 0x04 ? 32 : 1;
		return value;
	}
	}

	return 0;
}

uint8_t NesPPU::ReadVRAM(uint16_t addr)
{
	uint8_t value = 0;
	if (addr < 0x2000) {
		// Reading from CHR-ROM/RAM
		value = bus->cart->ReadCHR(addr);
	}
	else if (addr < 0x3F00) {
		// Reading from nametables and attribute tables
		uint16_t mirroredAddr = addr & 0x2FFF; // Mirror nametables every 4KB
		mirroredAddr = bus->cart->MirrorNametable(mirroredAddr);
		value = ppuDataBuffer;
		ppuDataBuffer = m_vram[mirroredAddr];
	}
	else if (addr < 0x4000) {
		// Reading from palette RAM (mirrored every 32 bytes)
		uint8_t paletteAddr = addr & 0x1F;
		if (paletteAddr % 4 == 0) {
			// Handle mirroring of the background color by setting the address to 0x3F00
			paletteAddr = 0;
		}
		value = paletteTable[paletteAddr];
		// But buffer is filled with the underlying nametable byte
		uint16_t mirroredAddr = addr & 0x2FFF; // Mirror nametables every 4KB
		mirroredAddr = bus->cart->MirrorNametable(mirroredAddr);
		ppuDataBuffer = m_vram[mirroredAddr];
	}

	return value;
}

void NesPPU::write_vram(uint16_t addr, uint8_t value)
{
	addr &= 0x3FFF; // Mask to 14 bits
	if (addr < 0x2000) {
		// Write to CHR-RAM (if enabled)
		bus->cart->WriteCHR(addr, value);
		// Else ignore write (CHR-ROM is typically read-only)
		return;
	}
	else if (addr < 0x3F00) {
		// Name tables and attribute tables
		addr &= 0x2FFF; // Mirror nametables every 4KB
		addr = bus->cart->MirrorNametable(addr);
		m_vram[addr] = value;
		return;
	}
	else if (addr < 0x4000) {
		// Palette RAM (mirrored every 32 bytes)
		// 3F00 = 0011 1111 0000 0000
		// 3F1F = 0011 1111 0001 1111
		uint8_t paletteAddr = addr & 0x1F; // 0001 1111
		if (paletteAddr % 4 == 0) {
			// Handle mirroring of the background color by setting the address to 0x3F00
			paletteAddr = 0;
		}
		paletteTable[paletteAddr] = value;
		InvalidateRect(core->m_hwndPalette, NULL, FALSE); // Update palette window if open
		return;
	}
}

// OAM DMA - Direct Memory Access for sprites
//void NesPPU::OAMDMA(uint8_t* cpuMemory, uint16_t page) {
//	uint16_t addr = page << 8;
//	for (int i = 0; i < 256; i++) {
//		oam[oamAddr++] = cpuMemory[addr + i];
//	}
//	// OAM DMA takes 513 or 514 CPU cycles depending on odd/even alignment
//	// OAMADDR wraps around automatically in hardware
//	oamAddr &= 0xFF;
//}

void NesPPU::RenderScanline()
{
	int scrollX = m_scrollX & 0xFF; // Fine X scrolling (0-255)
	int scrollY = m_scrollY & 0xFF; // Fine Y scrolling (0-239)
	int fineY = (m_scanline + scrollY) % (NAMETABLE_HEIGHT * TILE_SIZE); // Wrap around vertically
	// Render a single scanline to the back buffer here
	for (int screenX = 0; screenX < 256; ++screenX)
	{
		int fineX = (scrollX + screenX) % (NAMETABLE_WIDTH * TILE_SIZE);
		// Compute the base nametable index based on coarse scroll
		uint16_t coarseX = (scrollX + screenX) / (NAMETABLE_WIDTH * TILE_SIZE);
		uint16_t coarseY = (scrollY + m_scanline) / (NAMETABLE_HEIGHT * TILE_SIZE);

		// Determine which nametable we’re in (0–3)
		uint8_t nametableSelect = (m_ppuCtrl & 0x03);
		if (coarseX % 2) nametableSelect ^= 1;       // Switch horizontally
		if (coarseY % 2) nametableSelect ^= 2;       // Switch vertically

		uint16_t nametableAddr = 0x2000 + nametableSelect * 0x400;
		nametableAddr = bus->cart->MirrorNametable(nametableAddr);

		// Convert to tile coordinates
		int tileCol = fineX / TILE_SIZE;
		int tileRow = fineY / TILE_SIZE;
		int tileIndex = m_vram[nametableAddr + tileRow * NAMETABLE_WIDTH + tileCol];
		if (tileIndex == 0x48) {
			int test = 0;
		}
		else if (tileIndex > 0) {
			int test = 0;
		}
		// Get attribute byte for the tile
		int attrRow = tileRow / 4;
		int attrCol = tileCol / 4;
		uint8_t attributeByte = m_vram[(nametableAddr | 0x3c0) + attrRow * 8 + attrCol];

		uint8_t paletteIndex = 0;
		get_palette_index_from_attribute(attributeByte, tileRow, tileCol, paletteIndex);

		std::array<uint32_t, 4> palette;
		get_palette(paletteIndex, palette); // For now we don't use the colors

		// Get the color of the specific pixel within the tile
		int bgPixelInTileX = fineX % TILE_SIZE;
		int bgPixelInTileY = fineY % TILE_SIZE;
		uint8_t bgColorIndex = get_tile_pixel_color_index(tileIndex, bgPixelInTileX, bgPixelInTileY, false);
		// Handle both background and sprite color mapping here since we have to deal with
		// transparency and priority
		// Added bonus: Single draw call to set pixel in back buffer
		uint32_t bgColor = 0;
		if (bgColorIndex == 0) {
			bgColor = m_nesPalette[paletteTable[0]]; // Transparent color (background color)
		}
		else {
			bgColor = palette[bgColorIndex]; // Map to actual color from palette
		}
		// Set pixel in back buffer
		if (m_ppuMask & PPUMASK_BACKGROUNDENABLED) {
			m_backBuffer[(m_scanline * 256) + screenX] = bgColor;
		}
		//m_backBuffer[(m_scanline * 256) + screenX] = 0;
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
				int spritePixelY = m_scanline - spriteY;

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
				std::array<uint32_t, 4> spritePalette;
				get_palette(spritePaletteIndex + 4, spritePalette); // Sprite palettes start at 0x3F10
				// Get color index from sprite tile
				uint8_t spriteColorIndex = get_tile_pixel_color_index(sprite.tileIndex, spritePixelX, spritePixelY, true);
				if (spriteColorIndex != 0) { // Non-transparent pixel
					uint32_t spriteColor = spritePalette[spriteColorIndex];
					// Handle priority (not implemented yet, assuming sprites are always on top)
					if (sprite.isSprite0 && m_ppuMask & PPUMASK_RENDERINGEITHER) {
						// Sprite 0 hit detection
						// The sprite 0 hit flag is immediately set when any opaque pixel of sprite 0 overlaps
						// any opaque pixel of background, regardless of sprite priority.
						if (!hasSprite0HitBeenSet && bgColorIndex != 0) {
							hasSprite0HitBeenSet = true;
							m_ppuStatus |= PPUSTATUS_SPRITE0_HIT;
						}
					}
					if (m_ppuMask & PPUMASK_SPRITEENABLED) {
						m_backBuffer[(m_scanline * 256) + screenX] = spriteColor;
					}
					foundSprite = true;
				}
			}
		}
	}
}

void NesPPU::Clock() {
	// Emulate one PPU clock cycle here
	if (m_scanline >= 0 && m_scanline < 240) {
		//OutputDebugStringW((L"PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());
		//OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());
		if (m_cycle == 256) {
			//OutputDebugStringW((L"Rendering scanline " + std::to_wstring(m_scanline)
			//	+ L" at CPU Cycle " + std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
			// TODO: Pixel by pixel rendering can be implemented here for more accuracy
			RenderScanline();
		}
	}

	// VBlank scanlines (241-260)
	if (m_scanline == 241 && m_cycle == 1) {
		m_ppuStatus |= PPUSTATUS_VBLANK; // Set VBlank flag
		m_frameComplete = true;
		if (m_ppuCtrl & NMI_ENABLE) {
			// Trigger NMI if enabled
			//OutputDebugStringW((L"Triggering NMI at CPU cycle "
			//	+ std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
			bus->cpu->NMI();
		}
	}

	if (m_scanline == 50 && m_cycle == 1) {
		OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());
	}

	// Pre-render scanline (261)
	if (m_scanline == 261 && m_cycle == 1) {
		OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());

		//OutputDebugStringW((L"PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());

		//OutputDebugStringW((L"Pre-render scanline hit at CPU cycle "
		//	+ std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
		hasOverflowBeenSet = false;
		hasSprite0HitBeenSet = false;
		m_ppuStatus &= 0x1F; // Clear VBlank, sprite 0 hit, and sprite overflow
		m_frameComplete = false;
		m_backBuffer.fill(0xFF000000); // Clear back buffer to opaque black
	}

	// Advance cycle and scanline counters
	m_cycle++;
	if (m_cycle > 340) {
		// As per hardware behavior, evaluate sprites for the next scanline here
		EvaluateSprites(m_scanline, secondaryOAM);
		m_cycle = 0;
		m_scanline++;
		if (m_scanline > 261) {
			m_scanline = 0;
		}
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
			if (m_scanline == 0) {
				int test = 0;
			}
			if (spriteCount < 8) {
				// Copy sprite data to new OAM
				newOam[spriteCount].y = oam[i * 4];
				newOam[spriteCount].tileIndex = oam[i * 4 + 1];
				newOam[spriteCount].attributes = oam[i * 4 + 2];
				newOam[spriteCount].x = oam[i * 4 + 3];
				newOam[spriteCount].isSprite0 = (i == 0); // Mark if this is sprite 0
				spriteCount++;
			}
			else {
				// Sprite overflow - more than 8 sprites on this scanline
				if (!hasOverflowBeenSet) {
					// Set sprite overflow flag only once per frame
					hasOverflowBeenSet = true;
					m_ppuStatus |= PPUSTATUS_SPRITE_OVERFLOW;
				}
				break;
			}
		}
	}
}

// Render the entire frame (256x240 pixels). For testing purposes only.
// TODO : Possible optimization: Use dirty rectangles or only render changed areas to 
// a nametable buffer. Nametables don't change often, so we can cache them.
// We can then composite the nametable buffer with sprites and scrolling to produce the final frame.
// The nametable can be blitted via hardware acceleration for better performance.
// The biggest caviate is what to do if the CPU writes to the nametable while we're "rendering" it.
// Accuracy vs Performance tradeoff.
void NesPPU::render_frame()
{
	// Clear back buffer
	// m_backBuffer.fill(0xFF000000);
	int scrollX = m_scrollX & 0xFF; // Fine X scrolling (0-255)
	int scrollY = m_scrollY & 0xFF; // Fine Y scrolling (0-239)

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
			nametableAddr = bus->cart->MirrorNametable(nametableAddr);

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

			std::array<uint32_t, 4> palette;
			get_palette(paletteIndex, palette); // For now we don't use the colors

			// Get the color of the specific pixel within the tile
			int bgPixelInTileX = fineX % TILE_SIZE;
			int bgPixelInTileY = fineY % TILE_SIZE;
			uint8_t bgColorIndex = get_tile_pixel_color_index(tileIndex, bgPixelInTileX, bgPixelInTileY, false);
			// Handle both background and sprite color mapping here since we have to deal with
			// transparency and priority
			// Added bonus: Single draw call to set pixel in back buffer
			uint32_t bgColor = 0;
			if (bgColorIndex == 0) {
				bgColor = m_nesPalette[paletteTable[0]]; // Transparent color (background color)
			}
			else {
				bgColor = palette[bgColorIndex]; // Map to actual color from palette
			}
			// Set pixel in back buffer
			//m_backBuffer[(screenY * 256) + screenX] = bgColor;
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
					std::array<uint32_t, 4> spritePalette;
					get_palette(spritePaletteIndex + 4, spritePalette); // Sprite palettes start at 0x3F10
					// Get color index from sprite tile
					uint8_t spriteColorIndex = get_tile_pixel_color_index(sprite.tileIndex, spritePixelX, spritePixelY, true);
					if (spriteColorIndex != 0) { // Non-transparent pixel
						uint32_t spriteColor = spritePalette[spriteColorIndex];
						// Handle priority (not implemented yet, assuming sprites are always on top)
						//m_backBuffer[(screenY * 256) + screenX] = spriteColor;
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

bool NesPPU::NMI() {
	bool hasVBlank = m_ppuStatus & PPUSTATUS_VBLANK;
	if (hasVBlank) {
		// Disable blanking after NMI is triggered
		m_ppuStatus &= ~PPUSTATUS_VBLANK;
	}
	return hasVBlank && (m_ppuCtrl & 0x80);
}

uint8_t NesPPU::get_tile_pixel_color_index(uint8_t tileIndex, uint8_t pixelInTileX, uint8_t pixelInTileY, bool isSprite)
{
	// Determine the pattern table base address
	uint16_t patternTableBase = isSprite ? GetSpritePatternTableBase() : GetBackgroundPatternTableBase();
	int tileBase = patternTableBase + (tileIndex * 16); // 16 bytes per tile

	uint8_t byte1 = bus->cart->ReadCHR(tileBase + pixelInTileY);     // bitplane 0
	uint8_t byte2 = bus->cart->ReadCHR(tileBase + pixelInTileY + 8); // bitplane 1

	uint8_t bit0 = (byte1 >> (7 - pixelInTileX)) & 1;
	uint8_t bit1 = (byte2 >> (7 - pixelInTileX)) & 1;
	uint8_t colorIndex = (bit1 << 1) | bit0;

	if (colorIndex != 0)
	{
		int i = 0;
	}

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
	std::array<uint32_t, 4> palette;
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

void NesPPU::get_palette(uint8_t paletteIndex, std::array<uint32_t, 4>& colors)
{
	// Each palette consists of 4 colors, starting from 0x3F00 in VRAM
	uint16_t paletteAddr = paletteIndex * 4;
	colors[0] = m_nesPalette[paletteTable[paletteAddr] & 0x3F];
	colors[1] = m_nesPalette[paletteTable[paletteAddr + 1] & 0x3F];
	colors[2] = m_nesPalette[paletteTable[paletteAddr + 2] & 0x3F];
	colors[3] = m_nesPalette[paletteTable[paletteAddr + 3] & 0x3F];
}

void NesPPU::render_chr_rom()
{
	//m_backBuffer.fill(0xFF000000); // For testing, fill with opaque black
	std::array<uint32_t, 4> palette;
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

void NesPPU::render_tile(int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors) {
	int tileBase = tileIndex * 16; // 16 bytes per tile

	for (int y = 0; y < 8; y++) {
		uint8_t byte1 = bus->cart->ReadCHR(tileBase + y);     // bitplane 0
		uint8_t byte2 = bus->cart->ReadCHR(tileBase + y + 8); // bitplane 1

		for (int x = 0; x < 8; x++) {
			uint8_t bit0 = (byte1 >> (7 - x)) & 1;
			uint8_t bit1 = (byte2 >> (7 - x)) & 1;
			uint8_t colorIndex = (bit1 << 1) | bit0;

			uint16_t actualColor = 0;
			if (colorIndex == 0) {
				actualColor = m_nesPalette[paletteTable[0]]; // Transparent color (background color)
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