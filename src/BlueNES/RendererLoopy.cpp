#include "RendererLoopy.h"

// NES PPU Loopy Registers Implementation
// Based on the loopy register behavior documented by loopy

#include <stdint.h>
#include <stdbool.h>
#include "nes_ppu.h"
#include "Core.h"
#include "Bus.h"

void RendererLoopy::initialize(NesPPU* ppu) {
    m_ppu = ppu;
    m_bus = &ppu->core->bus;
}

void RendererLoopy::reset() {
    *(uint16_t*)&loopy.v = 0;
    *(uint16_t*)&loopy.t = 0;
    loopy.x = 0;
    loopy.w = false;
}

// Write to PPUCTRL ($2000)
void RendererLoopy::setPPUCTRL(uint8_t value) {
    // t: ...GH.. ........ <- d: ......GH
    // G = bit 1, H = bit 0 (nametable select)
    loopy.t.nametable_x = (value >> 0) & 1;
    loopy.t.nametable_y = (value >> 1) & 1;
}

// Write to PPUSCROLL ($2005)
void RendererLoopy::writeScroll(uint8_t value) {
    if (!loopy.w) {
        // First write (X scroll)
        // t: ....... ...HGFED <- d: HGFED...
        // x:              CBA <- d: .....CBA
        loopy.t.coarse_x = value >> 3;
        loopy.x = value & 0x07;
        loopy.w = true;
    }
    else {
        // Second write (Y scroll)
        // t: CBA..HG FED..... <- d: HGFEDCBA
        loopy.t.fine_y = value & 0x07;
        loopy.t.coarse_y = value >> 3;
        loopy.w = false;
    }
}

// Read from PPUSCROLL ($2005)
uint8_t RendererLoopy::getScrollY() {
    return ((loopy.t.coarse_y & 0x1F) << 3) | (loopy.t.fine_y & 0x07);
}

uint8_t RendererLoopy::getScrollX() {
    return ((loopy.t.coarse_x & 0x1F) << 3) | (loopy.x & 0x07);
}

// Write to PPUADDR ($2006)
void RendererLoopy::ppuWriteAddr(uint8_t value) {
    if (!loopy.w) {
        // First write (high byte)
        // t: .FEDCBA ........ <- d: ..FEDCBA
        // t: X...... ........ <- 0
        uint16_t* t_ptr = (uint16_t*)&loopy.t;
        *t_ptr = (*t_ptr & 0x00FF) | ((value & 0x3F) << 8);
        loopy.w = true;
    }
    else {
        // Second write (low byte)
        // t: ....... HGFEDCBA <- d: HGFEDCBA
        // v:                   <- t
        uint16_t* t_ptr = (uint16_t*)&loopy.t;
        *t_ptr = (*t_ptr & 0xFF00) | value;
        *(uint16_t*)&loopy.v = *(uint16_t*)&loopy.t;
        loopy.w = false;
    }
}

uint16_t RendererLoopy::getPPUAddr() {
    uint16_t* v_ptr = (uint16_t*)&loopy.v;
	return (*v_ptr & 0b11111111111111); // The address is 14 bits
}

// Read from PPUSTATUS ($2002)
void RendererLoopy::ppuReadStatus() {
    // w:                   <- 0
    loopy.w = false;
}

// Increment coarse X (only when rendering is enabled)
void RendererLoopy::ppuIncrementX() {
    if (loopy.v.coarse_x == 31) {
        loopy.v.coarse_x = 0;
        loopy.v.nametable_x ^= 1;  // Switch horizontal nametable
    }
    else {
        loopy.v.coarse_x++;
    }
}

// Increment Y (only when rendering is enabled)
void RendererLoopy::ppuIncrementY() {
    if (loopy.v.fine_y < 7) {
        loopy.v.fine_y++;
    }
    else {
        loopy.v.fine_y = 0;

        if (loopy.v.coarse_y == 29) {
            loopy.v.coarse_y = 0;
            loopy.v.nametable_y ^= 1;  // Switch vertical nametable
        }
        else if (loopy.v.coarse_y == 31) {
            loopy.v.coarse_y = 0;
            // Don't switch nametable (out of bounds)
        }
        else {
            loopy.v.coarse_y++;
        }
    }
}

// Copy horizontal bits from t to v (only when rendering is enabled)
void RendererLoopy::ppuCopyX() {
    // v: ....F.. ...EDCBA <- t: ....F.. ...EDCBA
    loopy.v.nametable_x = loopy.t.nametable_x;
    loopy.v.coarse_x = loopy.t.coarse_x;
}

// Copy vertical bits from t to v (only when rendering is enabled)
void RendererLoopy::ppuCopyY() {
    // v: IHGF.ED CBA..... <- t: IHGF.ED CBA.....
    loopy.v.fine_y = loopy.t.fine_y;
    loopy.v.nametable_y = loopy.t.nametable_y;
    loopy.v.coarse_y = loopy.t.coarse_y;
}

// Get current VRAM address
uint16_t RendererLoopy::ppuGetVramAddr() {
    return *(uint16_t*)&loopy.v & 0x3FFF;  // 14-bit address
}

// Increment VRAM address (after PPUDATA read/write)
void RendererLoopy::ppuIncrementVramAddr(uint8_t increment) {
    uint16_t* v_ptr = (uint16_t*)&loopy.v;
    *v_ptr = (*v_ptr + increment) & 0x7FFF;
}

// Get pixel from shift registers using fine X scroll
uint8_t RendererLoopy::get_pixel() {
    // Select the bit based on fine_x (0-7)
    // Bit 15 is leftmost, bit 0 is rightmost after shifting
    uint16_t mux = 0x8000 >> loopy.x;
    if ((m_shifts.pattern_lo_shift & 0x8000) != 0) {
		int test = 0;
    }

    // Extract pixel value (2 bits from pattern planes)
    uint8_t pixel = ((m_shifts.pattern_lo_shift & mux) ? 1 : 0) |
        ((m_shifts.pattern_hi_shift & mux) ? 2 : 0);

    // Extract palette (2 bits from attribute planes)
    uint8_t palette = ((m_shifts.attr_lo_shift & (0x80 >> loopy.x)) ? 1 : 0) |
        ((m_shifts.attr_hi_shift & (0x80 >> loopy.x)) ? 2 : 0);

    // Combine into final palette index (0-15)
    if (pixel == 0) return 0;  // Transparent
    return (palette << 2) | pixel;
}

void RendererLoopy::renderPixel() {
    int x = dot - 1; // visible pixel x [0..255]
    int y = m_scanline; // pixel y [0..239]
    
    uint8_t bgPaletteIndex = 0;
    uint32_t bgColor = 0;
    bool bgOpaque = false;
    if (bgEnabled()) {
        // TODO Set these
        // 
        // Get attribute byte for the tile
        //uint8_t nametableSelect = (loopy.v.nametable_x & 1) | ((loopy.v.nametable_y & 1) << 1);
        //uint16_t nametableAddr = 0x2000 + nametableSelect * 0x400;
        //nametableAddr = m_bus->cart->MirrorNametable(nametableAddr);
        //int attrRow = loopy.v.coarse_y / 4;
        //int attrCol = loopy.v.coarse_x / 4;
        //attributeByte = m_ppu->m_vram[(nametableAddr | 0x3c0) + attrRow * 8 + attrCol];
        //palette;
        //// CHR-ROM/RAM data for tile
        //chrLowByte;
        //chrHighByte;
        //m_backBuffer[(y * 256) + x] = finalColor;
    }
    // Render pixel using fine X scroll
    uint8_t pixel = get_pixel();
    uint8_t paletteIndex = m_ppu->paletteTable[pixel];
    uint32_t finalColor = m_nesPalette[paletteIndex];
    m_backBuffer[(y * 256) + x] = finalColor;
    //m_backBuffer[(y * 256) + x] = 0xFF0C9300;
}

// Get attribute byte for current tile
uint8_t RendererLoopy::get_attribute_byte() {
    // Attribute table starts at +0x3C0 from nametable base
    uint16_t nametable_base = 0x2000 | ((*(uint16_t*)&loopy.v & 0x0C00));
    uint16_t attr_addr = nametable_base | 0x03C0 |
        ((loopy.v.coarse_y >> 2) << 3) |
        (loopy.v.coarse_x >> 2);

    // Apply mirroring
    //attr_addr = m_bus->cart->MirrorNametable(attr_addr);
    return m_ppu->ReadVRAM(attr_addr);
}

// Get palette index from attribute byte
uint8_t RendererLoopy::get_palette_from_attribute(uint8_t attr, uint8_t coarse_x, uint8_t coarse_y) {
    // Each attribute byte covers a 4x4 tile area (32x32 pixels)
    // Divided into four 2x2 tile quadrants
    uint8_t quadrant_x = (coarse_x >> 1) & 1;
    uint8_t quadrant_y = (coarse_y >> 1) & 1;
    uint8_t shift = (quadrant_y << 2) | (quadrant_x << 1);
    return (attr >> shift) & 0x03;
}

// Fetch tile data at current v address
void RendererLoopy::fetch_tile_data(TileFetch* tile, uint8_t pattern_table_base) {
    // 1. Fetch nametable byte (tile index)
    uint16_t nametable_addr = 0x2000 | (*(uint16_t*)&loopy.v & 0x0FFF);
    tile->nametable_byte = m_ppu->ReadVRAM(nametable_addr);

    // TODO 2. Fetch attribute byte
    tile->attribute_byte = get_attribute_byte();

    // 3. Fetch pattern table low byte
    // Pattern table address = (pattern_base * 0x1000) + (tile_index * 16) + fine_y
    uint16_t pattern_addr = (pattern_table_base << 12) |
        (tile->nametable_byte << 4) |
        loopy.v.fine_y;
    tile->pattern_low = m_ppu->ReadVRAM(pattern_addr);

    // 4. Fetch pattern table high byte (+8 bytes from low)
    tile->pattern_high = m_ppu->ReadVRAM(pattern_addr + 8);
}

// Load fetched tile into shift registers
void RendererLoopy::load_shift_registers() {
    // Pattern data gets loaded into the lower 8 bits
    m_shifts.pattern_lo_shift = (m_shifts.pattern_lo_shift & 0xFF00) | tile.pattern_low;
    m_shifts.pattern_hi_shift = (m_shifts.pattern_hi_shift & 0xFF00) | tile.pattern_high;

    // Extract palette bits for this tile
    uint8_t palette = get_palette_from_attribute(tile.attribute_byte,
        loopy.v.coarse_x,
        loopy.v.coarse_y);
    m_shifts.attr_latch_lo = (palette & 1) ? 0xFF : 0x00;
    m_shifts.attr_latch_hi = (palette & 2) ? 0xFF : 0x00;
}

// Shift the registers (happens every cycle during rendering)
void RendererLoopy::shift_registers() {
    m_shifts.pattern_lo_shift <<= 1;
    m_shifts.pattern_hi_shift <<= 1;
    m_shifts.attr_lo_shift <<= 1;
    m_shifts.attr_hi_shift <<= 1;
}

// Reload attribute shift registers every 8 pixels
void RendererLoopy::reload_attribute_shift() {
    m_shifts.attr_lo_shift = m_shifts.attr_latch_lo;
    m_shifts.attr_hi_shift = m_shifts.attr_latch_hi;
}

void RendererLoopy::clock() {
    bool rendering = renderingEnabled();
    bool visibleScanline = (m_scanline >= 0 && m_scanline <= 239);
    bool preRenderLine = (m_scanline == 261);

    if (rendering) {
        if (((dot - 1) & 7) == 0) {
            // Load previous fetch into shift registers
            if (dot > 1) {
                load_shift_registers();
            }
            fetch_tile_data(&tile, m_ppu->GetBackgroundPatternTableBase() == 0x1000 ? 1 : 0);
            // Increment coarse X after fetching
            ppuIncrementX();

            // Reload attribute shift registers
            if (dot > 1) {
                reload_attribute_shift();
            }
        }
    }

    // Pixel rendering (visible)
    if (rendering && visibleScanline && dot >= 1 && dot <= 256) {
        // Shift registers every cycle
        shift_registers();
        renderPixel();
    }
    else if ((preRenderLine || visibleScanline) && dot >= 321 && dot <= 336 && ((dot - 1) & 7) == 0) {
        // While in 321..336 we still load shifters and such for the next scanline,
        // but don't render pixels (this is part of the fetch window).
        // shift each dot as well to keep pipeline in sync.
		// This ensures that when we start rendering, we have the correct data in the shifters.
		// With two tiles fetched ahead, we can render the first pixel of the next scanline correctly.
        shift_registers();
    }

    // VBlank scanlines (241-260)
    if (m_scanline == 241 && dot == 1) {
        m_ppu->m_ppuStatus |= PPUSTATUS_VBLANK; // Set VBlank flag
        m_frameComplete = true;
        if (m_ppu->m_ppuCtrl & NMI_ENABLE) {
            // Trigger NMI if enabled
            //OutputDebugStringW((L"Triggering NMI at CPU cycle "
            //	+ std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
            m_bus->cpu->NMI();
        }
    }

    if (m_scanline == 50 && dot == 1) {
        //OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(ppu->m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(GetScrollX()) + L"\n").c_str());
    }

    // Pre-render scanline (261)
    if (preRenderLine && dot == 1) {
        //OutputDebugStringW((L"Scanline: " + std::to_wstring(m_scanline) + L", nametable: " + std::to_wstring(ppu->m_ppuCtrl & 3) + L", PPU Scroll X: " + std::to_wstring(GetScrollX()) + L"\n").c_str());

        //OutputDebugStringW((L"PPU Scroll X: " + std::to_wstring(m_scrollX) + L"\n").c_str());

        //OutputDebugStringW((L"Pre-render scanline hit at CPU cycle "
        //	+ std::to_wstring(bus->cpu->cyclesThisFrame) + L"\n").c_str());
        hasOverflowBeenSet = false;
        hasSprite0HitBeenSet = false;
        m_ppu->m_ppuStatus &= 0x1F; // Clear VBlank, sprite 0 hit, and sprite overflow
        m_frameComplete = false;
        //m_backBuffer.fill(0xFF000000); // Clear back buffer to opaque black
        //OutputDebugStringW((L"PPUCTRL at render: " + std::to_wstring(ppu->m_ppuCtrl) + L"\n").c_str());
    }
    //    // On dot 256: increment Y
    if (rendering && dot == 256 && (visibleScanline || preRenderLine)) {
        ppuIncrementY();
    }

    // On dot 257: copy horizontal bits from t to v and start sprite evaluation
    if (rendering && dot == 257 && (visibleScanline || preRenderLine)) {
        ppuCopyX();
    }
    if (rendering && dot == 257) {
        evaluateSprites(m_scanline, secondaryOAM);
    }
 
    // Pre-render only: dots 280..304 copy vertical bits from t to v
    if (preRenderLine && dot >= 280 && dot <= 304 && rendering) {
        ppuCopyY();
    }

    // Advance cycle and scanline counters
    //m_cycle++;
    //if (m_cycle > 340) {
    //    m_cycle = 0;
    //    m_scanline++;
    //    if (m_scanline > 261) {
    //        m_scanline = 0;
    //    }
    //}

    dot++;
    if (dot >= DOTS_PER_SCANLINE) {
        dot = 1;
        m_scanline++;
        if (m_scanline > SCANLINES_PER_FRAME) {
            m_scanline = 0;
        }
    }
}


void RendererLoopy::evaluateSprites(int screenY, std::array<Sprite, 8>& newOam) {
    for (int i = 0; i < 8; ++i) {
        newOam[i] = { 0xFF, 0xFF, 0xFF, 0xFF }; // Initialize to empty sprite
    }
    // Evaluate sprites for this scanline
    int spriteCount = 0;
    for (int i = 0; i < 64; ++i) {
        int spriteY = m_ppu->oam[i * 4]; // Y position of the sprite
        if (spriteY > 0xF0) {
            continue; // Empty sprite slot
        }
        int spriteHeight = (m_ppu->m_ppuCtrl & PPUCTRL_SPRITESIZE) == 0 ? 8 : 16;
        if (screenY >= spriteY && screenY < (spriteY + spriteHeight)) {
            if (m_scanline == 0) {
                int test = 0;
            }
            if (spriteCount < 8) {
                // Copy sprite data to new OAM
                newOam[spriteCount].y = m_ppu->oam[i * 4];
                newOam[spriteCount].tileIndex = m_ppu->oam[i * 4 + 1];
                newOam[spriteCount].attributes = m_ppu->oam[i * 4 + 2];
                newOam[spriteCount].x = m_ppu->oam[i * 4 + 3];
                newOam[spriteCount].isSprite0 = (i == 0); // Mark if this is sprite 0
                spriteCount++;
            }
            else {
                // Sprite overflow - more than 8 sprites on this scanline
                if (!hasOverflowBeenSet) {
                    // Set sprite overflow flag only once per frame
                    hasOverflowBeenSet = true;
                    m_ppu->m_ppuStatus |= PPUSTATUS_SPRITE_OVERFLOW;
                }
                break;
            }
        }
    }
}