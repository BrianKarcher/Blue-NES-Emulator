#include "RendererLoopy.h"

// NES PPU Loopy Registers Implementation
// Based on the loopy register behavior documented by loopy

#include <stdint.h>
#include <stdbool.h>
#include "nes_ppu.h"

void RendererLoopy::initialize(NesPPU* ppu) {
    m_ppu = ppu;
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