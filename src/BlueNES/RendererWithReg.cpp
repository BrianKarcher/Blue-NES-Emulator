#include "RendererWithReg.h"
#include "Core.h"
#include "Bus.h"
#include "nes_ppu.h"

RendererWithReg::RendererWithReg() {
	v = 0;
	t = 0;
	w = false;
	x = 0;
	m_backBuffer.fill(0xFF000000); // Initialize to opaque black
}

//void RenderWithReg::SetW(bool w) {
//	this->w = w;
//}

void RendererWithReg::Initialize(NesPPU* ppu) {
	this->ppu = ppu;
	bus = &ppu->core->bus;
}

void RendererWithReg::SetPPUCTRL(uint8_t value) {
	// Set nametable bits in register t
	t = (t & ~INTERNAL_NAMETABLE) | ((value & 0x03) << 10);
	vramIncrementRight = !(value & 0b100);
}

void RendererWithReg::SetScrollX(uint8_t value) {
	// Coarse X goes into bits 0-4 of t
	t = (t & ~INTERNAL_COARSE_X) | ((value >> 3) & 0x1F);
	// Fine X goes into the x register
	x = value & 0x07;
}

void RendererWithReg::SetScrollY(uint8_t value) {
	// Fine Y goes into bits 12-14 of t
	t = (t & ~INTERNAL_FINE_Y) | (((uint16_t)(value & 0x07)) << 12);
	// Coarse Y goes into bits 5-9 of t
	t = (t & ~INTERNAL_COARSE_Y) | (((uint16_t)(value >> 3) & 0x1F) << 5);
}

uint8_t RendererWithReg::GetScrollX()
{
	// Reconstruct scroll X from coarse X (bits 0-4) and fine X register
	return ((t & INTERNAL_COARSE_X) << 3) | (x & 0x07);
}

uint8_t RendererWithReg::GetScrollY()
{
	// Reconstruct scroll Y from coarse Y (bits 5-9) and fine Y (bits 12-14)
	uint8_t coarseY = (t & INTERNAL_COARSE_Y) >> 5;
	uint8_t fineY = (t & INTERNAL_FINE_Y) >> 12;
	return (coarseY << 3) | fineY;
}

void RendererWithReg::SetPPUAddrHigh(uint8_t value) {
	// PPUADDR first write: t = (t & 0x00FF) | ((value & 0x3F) << 8)
	// This sets bits 8-13, clearing bit 14
	t = (t & 0x00FF) | (((uint16_t)(value & 0x3F)) << 8);
}

void RendererWithReg::SetPPUAddrLow(uint8_t value) {
	// PPUADDR second write: t = (t & 0xFF00) | value, then v = t
	t = (t & 0xFF00) | value;
	v = t;
}

void RendererWithReg::SetWriteToggle(bool toggle) {
	w = toggle;
}

bool RendererWithReg::GetWriteToggle() const {
	return w;
}

uint16_t RendererWithReg::GetPPUAddr() {
	return (v & 0b11111111111111); // The address is 14 bits
}

void RendererWithReg::PPUDataAccess() {
	// TODO Handle data access during rendering (where it glitches up)
	//v += 1;
	v += vramIncrementRight ? 1 : 32;
	//uint16_t addrTemp = GetPPUAddr();
	//if (addrTemp >= 0x3F00) {
	//	// Palette data always increments by 1
	//	v += 1;
	//	return;
	//}
	//// Increment VRAM address based on PPUCTRL setting
	//v += vramIncrementRight ? 1 : 32;
}

//void RendererSlow::render_chr_rom()
//{
//	//m_backBuffer.fill(0xFF000000); // For testing, fill with opaque black
//	std::array<uint32_t, 4> palette;
//	// Draw a whole pattern table for testing
//	for (int pr = 0; pr < 128; pr += 8) {
//		for (int pc = 0; pc < 128; pc += 8) {
//			int tileRow = pr / 8;
//			int tileCol = pc / 8;
//			int tileIndex = (tileRow * 16) + tileCol;
//			render_tile(pr, pc, tileIndex, palette);
//		}
//	}
//}

void RendererWithReg::clock() {
	bool rendering = renderingEnabled();
    bool visibleScanline = (m_scanline >= 0 && m_scanline <= 239);
    bool preRenderLine = (m_scanline == 261);
 
	// Emulate one PPU clock cycle here
	if (m_scanline >= 0 && m_scanline < 240) {
		//OutputDebugStringW((L"PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());
		//OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());
		if (m_cycle == 256 && rendering) {
			//OutputDebugStringW((L"Rendering scanline " + std::to_wstring(m_scanline)
			//	+ L" at CPU Cycle " + std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
			// TODO: Pixel by pixel rendering can be implemented here for more accuracy
			RenderScanline();
		}
	}

	// VBlank scanlines (241-260)
	if (m_scanline == 241 && m_cycle == 1) {
		ppu->m_ppuStatus |= PPUSTATUS_VBLANK; // Set VBlank flag
		m_frameComplete = true;
		if (ppu->m_ppuCtrl & NMI_ENABLE) {
			// Trigger NMI if enabled
			//OutputDebugStringW((L"Triggering NMI at CPU cycle "
			//	+ std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
			bus->cpu->NMI();
		}
	}

	if (m_scanline == 50 && m_cycle == 1) {
		//OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(ppu->m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(GetScrollX()) + L"\n").c_str());
	}

	// Pre-render scanline (261)
	if (m_scanline == 261 && m_cycle == 1) {
		//OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(ppu->m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(GetScrollX()) + L"\n").c_str());

		//OutputDebugStringW((L"PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());

		//OutputDebugStringW((L"Pre-render scanline hit at CPU cycle "
		//	+ std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
		hasOverflowBeenSet = false;
		hasSprite0HitBeenSet = false;
		ppu->m_ppuStatus &= 0x1F; // Clear VBlank, sprite 0 hit, and sprite overflow
		m_frameComplete = false;
		//m_backBuffer.fill(0xFF000000); // Clear back buffer to opaque black
		//OutputDebugStringW((L"PPUCTRL at render: " + std::to_wstring(ppu->m_ppuCtrl) + L"\n").c_str());
	}
	//    // On dot 256: increment Y
    if (rendering && m_cycle == 256 && (visibleScanline)) {
        //incrementY();
    }

    // On dot 257: copy horizontal bits from t to v and start sprite evaluation
    if (rendering && m_cycle == 257 && (visibleScanline)) {
        copyHorizontalBitsFromTtoV();
    }
	if (rendering && m_cycle == 257) {
		EvaluateSprites(m_scanline, secondaryOAM);
	}

    // Pre-render only: dots 280..304 copy vertical bits from t to v
    if (preRenderLine && m_cycle >= 280 && m_cycle <= 304 && rendering) {
        copyVerticalBitsFromTtoV();
    }

	// Advance cycle and scanline counters
	m_cycle++;
	if (m_cycle > 340) {
		m_cycle = 0;
		m_scanline++;
		if (m_scanline > 261) {
			m_scanline = 0;
		}
	}
}

void RendererWithReg::EvaluateSprites(int screenY, std::array<Sprite, 8>& newOam) {
	for (int i = 0; i < 8; ++i) {
		newOam[i] = { 0xFF, 0xFF, 0xFF, 0xFF }; // Initialize to empty sprite
	}
	// Evaluate sprites for this scanline
	int spriteCount = 0;
	for (int i = 0; i < 64; ++i) {
		int spriteY = ppu->oam[i * 4]; // Y position of the sprite
		if (spriteY > 0xF0) {
			continue; // Empty sprite slot
		}
		int spriteHeight = (ppu->m_ppuCtrl & PPUCTRL_SPRITESIZE) == 0 ? 8 : 16;
		if (screenY >= spriteY && screenY < (spriteY + spriteHeight)) {
			if (m_scanline == 0) {
				int test = 0;
			}
			if (spriteCount < 8) {
				// Copy sprite data to new OAM
				newOam[spriteCount].y = ppu->oam[i * 4];
				newOam[spriteCount].tileIndex = ppu->oam[i * 4 + 1];
				newOam[spriteCount].attributes = ppu->oam[i * 4 + 2];
				newOam[spriteCount].x = ppu->oam[i * 4 + 3];
				newOam[spriteCount].isSprite0 = (i == 0); // Mark if this is sprite 0
				spriteCount++;
			}
			else {
				// Sprite overflow - more than 8 sprites on this scanline
				if (!hasOverflowBeenSet) {
					// Set sprite overflow flag only once per frame
					hasOverflowBeenSet = true;
					ppu->m_ppuStatus |= PPUSTATUS_SPRITE_OVERFLOW;
				}
				break;
			}
		}
	}
}

void RendererWithReg::copyHorizontalBitsFromTtoV() {
    v = (v & ~0x041F) | (t & 0x041F);
}

void RendererWithReg::copyVerticalBitsFromTtoV() {
    v = (v & ~0x7BE0) | (t & 0x7BE0);
}

// ---------------- Loopy helpers ----------------
void RendererWithReg::incrementX() {
	// if coarse X == 31: coarse X = 0, switch horizontal nametable
	if ((v & 0x001F) == 31) {
		v &= ~0x001F;
		v ^= 0x0400;
	}
	else {
		v += 1;
	}
}

void RendererWithReg::incrementY() {
	// if fine Y < 7 -> fine Y++
	if ((v & 0x7000) != 0x7000) {
		v += 0x1000; // fine Y++
	}
	else {
		// fine Y = 0
		v &= ~0x7000;
		uint16_t y = (v & 0x03E0) >> 5; // coarse Y
		if (y == 29) {
			y = 0;
			v ^= 0x0800; // switch vertical nametable
		}
		else if (y == 31) {
			y = 0; // overflow to 0, no nametable switch
		}
		else {
			y++;
		}
		v = (v & ~0x03E0) | (y << 5);
	}
}

void RendererWithReg::RenderScanline()
{
	//return;
	int scrollX = (v & 0b11111) << 3 | (x & 0b111);
	int scrollY = ((v & 0b1111100000) >> 2) | ((v & 0b111000000000000) >> 12);
	// OutputDebugStringW((L"scrollX: " + std::to_wstring(scrollX) + L", scrollY: " + std::to_wstring(scrollY) + L"\n").c_str());
	//int scrollX = m_scrollX & 0xFF; // Fine X scrolling (0-255)
	//int scrollY = m_scrollY & 0xFF; // Fine Y scrolling (0-239)
	int fineY = (m_scanline + scrollY) % (NAMETABLE_HEIGHT * TILE_SIZE); // Wrap around vertically
	// Render a single scanline to the back buffer here
	for (int screenX = 0; screenX < 256; ++screenX)
	{
		uint8_t bgPaletteIndex = 0;
		uint32_t bgColor = 0;
		bool bgOpaque = false;
		if (bgEnabled()) {
			int fineX = (screenX + scrollX) % (NAMETABLE_WIDTH * TILE_SIZE);
			// Compute the base nametable index based on coarse scroll
			uint16_t coarseX = (scrollX + screenX) / (NAMETABLE_WIDTH * TILE_SIZE);
			uint16_t coarseY = (scrollY + m_scanline) / (NAMETABLE_HEIGHT * TILE_SIZE);

			// Determine which nametable we’re in (0–3)
			//uint8_t nametableSelect = (ppu->m_ppuCtrl & 0x03);
			uint8_t nametableSelect = (v & INTERNAL_NAMETABLE) >> 10;
			if (coarseX % 2) nametableSelect ^= 1;       // Switch horizontally
			if (coarseY % 2) nametableSelect ^= 2;       // Switch vertically

			uint16_t nametableAddr = 0x2000 + nametableSelect * 0x400;
			nametableAddr = bus->cart->MirrorNametable(nametableAddr);

			// Convert to tile coordinates
			int tileCol = fineX / TILE_SIZE;
			int tileRow = fineY / TILE_SIZE;
			int tileIndex = ppu->m_vram[nametableAddr + tileRow * NAMETABLE_WIDTH + tileCol];
			if (tileIndex == 0x48) {
				int test = 0;
			}
			else if (tileIndex > 0) {
				int test = 0;
			}
			// Get attribute byte for the tile
			int attrRow = tileRow / 4;
			int attrCol = tileCol / 4;
			uint8_t attributeByte = ppu->m_vram[(nametableAddr | 0x3c0) + attrRow * 8 + attrCol];

			uint8_t paletteIndex = 0;
			get_palette_index_from_attribute(attributeByte, tileRow, tileCol, paletteIndex);

			std::array<uint32_t, 4> palette;
			ppu->get_palette(paletteIndex, palette);

			// Get the color of the specific pixel within the tile
			int bgPixelInTileX = fineX % TILE_SIZE;
			int bgPixelInTileY = fineY % TILE_SIZE;
			uint8_t bgColorIndex = ppu->get_tile_pixel_color_index(tileIndex, bgPixelInTileX, bgPixelInTileY, false, false);
			// Handle both background and sprite color mapping here since we have to deal with
			// transparency and priority
			// Added bonus: Single draw call to set pixel in back buffer
			if (bgColorIndex % 4 == 0) {
				bgColor = m_nesPalette[ppu->paletteTable[0]]; // Transparent color (background color)
				bgOpaque = false;
			}
			else {
				bgColor = palette[bgColorIndex]; // Map to actual color from palette
				bgOpaque = true;
			}
			// Set pixel in back buffer
			/*if (ppumask & PPUMASK_BACKGROUNDENABLED) {
				m_backBuffer[(m_scanline * 256) + screenX] = bgColor;
			}*/
		}

		// Sprite pixel (simplified): we check secondaryOAM sprites for an opaque pixel at current x
		uint8_t spritePaletteIndex = 0;
		uint32_t spriteColor = 0;
		bool spriteOpaque = false;
		bool spritePriorityBehind = false;
		bool spriteIsZero = false;

		if (spriteEnabled()) {
			for (int i = 0; i < 8; ++i) {
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
						int spriteHeight = 7;
						if ((ppu->m_ppuCtrl & PPUCTRL_SPRITESIZE) != 0) {
							spriteHeight = 15;
						}
						spritePixelY = spriteHeight - spritePixelY;
					}
					// Get sprite palette
					uint8_t paletteIndex = sprite.attributes & 0x03;
					std::array<uint32_t, 4> spritePalette;
					ppu->get_palette(paletteIndex + 4, spritePalette); // Sprite palettes start at 0x3F10
					// Get color index from sprite tile
					bool isSecondSprite = spritePixelY >= 8;
					if (spritePixelY >= 8) {
						// 8x16 support.
						spritePixelY -= 8;
					}
					uint8_t spriteColorIndex = ppu->get_tile_pixel_color_index(sprite.tileIndex, spritePixelX, spritePixelY, true, isSecondSprite);
					if (spriteColorIndex != 0) { // Non-transparent pixel
						spritePaletteIndex = spriteColorIndex;
						spriteOpaque = true;
						spritePriorityBehind = (sprite.attributes & 0x20) != 0;
						spriteColor = spritePalette[spriteColorIndex];
						// Handle priority (not implemented yet, assuming sprites are always on top)
						if (sprite.isSprite0) {
							spriteIsZero = true;
							// Sprite 0 hit detection
							// The sprite 0 hit flag is immediately set when any opaque pixel of sprite 0 overlaps
							// any opaque pixel of background, regardless of sprite priority.
							/*if (!hasSprite0HitBeenSet && bgPaletteIndex != 0) {
								hasSprite0HitBeenSet = true;
								ppu->m_ppuStatus |= PPUSTATUS_SPRITE0_HIT;
							}*/
						}
						//if (ppumask & PPUMASK_SPRITEENABLED) {
						//	m_backBuffer[(m_scanline * 256) + screenX] = spriteColor;
						//}
						break;
					}
				}
			}
		}

		// Sprite0 hit: If sprite0 overlaps non-transparent background and both BG and sprites are enabled,
		// and x != 255, and sprite0 hasn't been flagged yet.
		if (!hasSprite0HitBeenSet && spriteIsZero && bgOpaque && spriteOpaque && bgEnabled() && spriteEnabled() && x != 255) {
			// Determine if sprite belonged to primary OAM index 0 on this scanline -- we simplified above
			// In a full implementation you should check whether that opaque sprite corresponds to original OAM slot 0.
			hasSprite0HitBeenSet = true;
			ppu->SetPPUStatus(0x40); // set Sprite 0 Hit
		}

		// Final pixel selection: sprite over bg depending on priority
		uint8_t finalPalette = 0;
		uint32_t finalColor = 0;
		if (spriteOpaque && (!bgOpaque || !spritePriorityBehind)) {
			// sprite wins
			finalPalette = spritePaletteIndex;
			finalColor = spriteColor;
		}
		else {
			finalPalette = bgPaletteIndex;
			finalColor = bgColor;
		}

		// Convert palette index to color (here we just read a byte from paletteTable and write to framebuffer)
		if (finalPalette != 0) {
			int i = 0;
		}
		//uint8_t color = paletteTable[finalPalette & 0x1F];
		// For demonstration store color value as ARGB-like 32-bit; user must map palette to RGB
		//uint32_t pixel32 = 0xFF000000 | (color * 0x010101); // grayish mapping; replace with actual palette RGB
		//uint32_t pixel32 = m_nesPalette[color];
		//if (x >= 0 && x < 256 && y >= 0 && y < 240) {
		m_backBuffer[(m_scanline * 256) + screenX] = finalColor;
		//}
	}
}

void RendererWithReg::get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex)
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

// Render the entire frame (256x240 pixels). For testing purposes only.
// TODO : Possible optimization: Use dirty rectangles or only render changed areas to 
// a nametable buffer. Nametables don't change often, so we can cache them.
// We can then composite the nametable buffer with sprites and scrolling to produce the final frame.
// The nametable can be blitted via hardware acceleration for better performance.
// The biggest caviate is what to do if the CPU writes to the nametable while we're "rendering" it.
// Accuracy vs Performance tradeoff.
//void RendererSlow::render_frame()
//{
//	// Clear back buffer
//	// m_backBuffer.fill(0xFF000000);
//	int scrollX = renderer->GetScrollX() & 0xFF; // Fine X scrolling (0-255)
//	int scrollY = renderer->GetScrollY() & 0xFF; // Fine Y scrolling (0-239)
//
//	for (int i = 0; i < 8; ++i) {
//		secondaryOAM[i] = { 0xFF, 0xFF, 0xFF, 0xFF }; // Initialize to empty sprite
//	}
//	uint8_t baseNametableIndex = (m_ppuCtrl & 0x03); // Bits 0-1 of PPUCTRL determine nametable base
//	uint16_t baseNametableAddr = 0x2000 + (baseNametableIndex * 0x400);
//	// Render the 256x240 visible area
//	for (int screenY = 0; screenY < 240; ++screenY)
//	{
//		// uint16_t currentNametableIndex = baseNametableAddr;
//		int fineY = (screenY + scrollY) % (NAMETABLE_HEIGHT * TILE_SIZE); // Wrap around vertically
//		//bool flipped = false;
//		for (int screenX = 0; screenX < 256; ++screenX)
//		{
//			int fineX = (scrollX + screenX) % (NAMETABLE_WIDTH * TILE_SIZE);
//			// Compute the base nametable index based on coarse scroll
//			uint16_t coarseX = (scrollX + screenX) / (NAMETABLE_WIDTH * TILE_SIZE);
//			uint16_t coarseY = (scrollY + screenY) / (NAMETABLE_HEIGHT * TILE_SIZE);
//
//			// Determine which nametable we’re in (0–3)
//			uint8_t nametableSelect = (m_ppuCtrl & 0x03);
//			if (coarseX % 2) nametableSelect ^= 1;       // Switch horizontally
//			if (coarseY % 2) nametableSelect ^= 2;       // Switch vertically
//
//			uint16_t nametableAddr = 0x2000 + nametableSelect * 0x400;
//			nametableAddr = bus->cart->MirrorNametable(nametableAddr);
//
//			// Convert to tile coordinates
//			int tileCol = fineX / TILE_SIZE;
//			int tileRow = fineY / TILE_SIZE;
//			int tileIndex = m_vram[nametableAddr + tileRow * NAMETABLE_WIDTH + tileCol];
//
//			// Get attribute byte for the tile
//			int attrRow = tileRow / 4;
//			int attrCol = tileCol / 4;
//			uint8_t attributeByte = m_vram[(nametableAddr | 0x3c0) + attrRow * 8 + attrCol];
//
//			uint8_t paletteIndex = 0;
//			get_palette_index_from_attribute(attributeByte, tileRow, tileCol, paletteIndex);
//
//			std::array<uint32_t, 4> palette;
//			get_palette(paletteIndex, palette); // For now we don't use the colors
//
//			// Get the color of the specific pixel within the tile
//			int bgPixelInTileX = fineX % TILE_SIZE;
//			int bgPixelInTileY = fineY % TILE_SIZE;
//			uint8_t bgColorIndex = get_tile_pixel_color_index(tileIndex, bgPixelInTileX, bgPixelInTileY, false);
//			// Handle both background and sprite color mapping here since we have to deal with
//			// transparency and priority
//			// Added bonus: Single draw call to set pixel in back buffer
//			uint32_t bgColor = 0;
//			if (bgColorIndex == 0) {
//				bgColor = m_nesPalette[paletteTable[0]]; // Transparent color (background color)
//			}
//			else {
//				bgColor = palette[bgColorIndex]; // Map to actual color from palette
//			}
//			// Set pixel in back buffer
//			//m_backBuffer[(screenY * 256) + screenX] = bgColor;
//			bool foundSprite = false;
//			for (int i = 0; i < 8 && !foundSprite; ++i) {
//				Sprite& sprite = secondaryOAM[i];
//				if (sprite.y >= 0xF0) {
//					continue; // Empty sprite slot
//				}
//				int spriteY = sprite.y + 1; // Adjust for off-by-one
//				int spriteX = sprite.x;
//				if (screenX >= spriteX && screenX < (spriteX + 8)) {
//					// Sprite pixel coordinates
//					int spritePixelX = screenX - spriteX;
//					int spritePixelY = screenY - spriteY;
//
//					bool flipHorizontal = sprite.attributes & 0x40;
//					bool flipVertical = sprite.attributes & 0x80;
//					if (flipHorizontal) {
//						spritePixelX = 7 - spritePixelX;
//					}
//					if (flipVertical) {
//						spritePixelY = 7 - spritePixelY;
//					}
//					// Get sprite palette
//					uint8_t spritePaletteIndex = sprite.attributes & 0x03;
//					std::array<uint32_t, 4> spritePalette;
//					get_palette(spritePaletteIndex + 4, spritePalette); // Sprite palettes start at 0x3F10
//					// Get color index from sprite tile
//					uint8_t spriteColorIndex = get_tile_pixel_color_index(sprite.tileIndex, spritePixelX, spritePixelY, true);
//					if (spriteColorIndex != 0) { // Non-transparent pixel
//						uint32_t spriteColor = spritePalette[spriteColorIndex];
//						// Handle priority (not implemented yet, assuming sprites are always on top)
//						//m_backBuffer[(screenY * 256) + screenX] = spriteColor;
//						foundSprite = true;
//					}
//				}
//			}
//		}
//		// Sprite evaluation for this scanline get rendered on the next scanline
//		// All sprites on the NES have an off-by-one error for the Y position
//		EvaluateSprites(screenY, secondaryOAM);
//	}
//}

//void RendererSlow::render_nametable()
//{
//	// Clear back buffer
//	// m_backBuffer.fill(0xFF000000);
//
//	// Nametable starts at 0x2000 in VRAM
//	// TODO : Support multiple nametables and mirroring
//	const uint16_t nametableAddr = 0x2000;
//
//	// TODO: For now, we just render the nametable directly without scrolling or attribute tables
//	std::array<uint32_t, 4> palette;
//	// Render the 32x30 tile nametable
//	for (int row = 0; row < 30; row++) {
//		for (int col = 0; col < 32; col++) {
//			// Get the tile index from the nametable in VRAM
//			uint8_t tileIndex = m_vram[nametableAddr + row * 32 + col];
//
//			// Calculate pixel coordinates
//			int pixelX = col * 8;  // Each tile is 8x8 pixels
//			int pixelY = row * 8;
//
//			int attrRow = row / 4;
//			int attrCol = col / 4;
//			// Get attribute byte for the tile
//			uint8_t attributeByte = m_vram[0x23c0 + attrRow * 8 + attrCol];
//
//			uint8_t paletteIndex = 0;
//			get_palette_index_from_attribute(attributeByte, row, col, paletteIndex);
//			get_palette(paletteIndex, palette); // For now we don't use the colors
//			// Render the tile at the calculated position
//			render_tile(pixelY, pixelX, tileIndex, palette);
//		}
//	}
//}