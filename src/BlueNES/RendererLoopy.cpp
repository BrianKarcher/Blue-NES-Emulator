#include "RendererLoopy.h"

// NES PPU Loopy Registers Implementation
// Based on the loopy register behavior documented by loopy

#include <stdint.h>
#include <stdbool.h>

void RendererLoopy::reset() {
    *(uint16_t*)&ppu.v = 0;
    *(uint16_t*)&ppu.t = 0;
    ppu.x = 0;
    ppu.w = false;
}

// Write to PPUCTRL ($2000)
void RendererLoopy::SetPPUCTRL(uint8_t value) {
    // t: ...GH.. ........ <- d: ......GH
    // G = bit 1, H = bit 0 (nametable select)
    ppu.t.nametable_x = (value >> 0) & 1;
    ppu.t.nametable_y = (value >> 1) & 1;
}

// Write to PPUSCROLL ($2005)
void RendererLoopy::WriteScroll(uint8_t value) {
    if (!ppu.w) {
        // First write (X scroll)
        // t: ....... ...HGFED <- d: HGFED...
        // x:              CBA <- d: .....CBA
        ppu.t.coarse_x = value >> 3;
        ppu.x = value & 0x07;
        ppu.w = true;
    }
    else {
        // Second write (Y scroll)
        // t: CBA..HG FED..... <- d: HGFEDCBA
        ppu.t.fine_y = value & 0x07;
        ppu.t.coarse_y = value >> 3;
        ppu.w = false;
    }
}

//// Write to PPUADDR ($2006)
//void ppu_write_addr(PPURegisters* ppu, uint8_t value) {
//    if (!ppu->w) {
//        // First write (high byte)
//        // t: .FEDCBA ........ <- d: ..FEDCBA
//        // t: X...... ........ <- 0
//        uint16_t* t_ptr = (uint16_t*)&ppu->t;
//        *t_ptr = (*t_ptr & 0x00FF) | ((value & 0x3F) << 8);
//        ppu->w = true;
//    }
//    else {
//        // Second write (low byte)
//        // t: ....... HGFEDCBA <- d: HGFEDCBA
//        // v:                   <- t
//        uint16_t* t_ptr = (uint16_t*)&ppu->t;
//        *t_ptr = (*t_ptr & 0xFF00) | value;
//        *(uint16_t*)&ppu->v = *(uint16_t*)&ppu->t;
//        ppu->w = false;
//    }
//}
//
//// Read from PPUSTATUS ($2002)
//void ppu_read_status(PPURegisters* ppu) {
//    // w:                   <- 0
//    ppu->w = false;
//}
//
//// Increment coarse X (only when rendering is enabled)
//void ppu_increment_x(PPURegisters* ppu) {
//    if (ppu->v.coarse_x == 31) {
//        ppu->v.coarse_x = 0;
//        ppu->v.nametable_x ^= 1;  // Switch horizontal nametable
//    }
//    else {
//        ppu->v.coarse_x++;
//    }
//}
//
//// Increment Y (only when rendering is enabled)
//void ppu_increment_y(PPURegisters* ppu) {
//    if (ppu->v.fine_y < 7) {
//        ppu->v.fine_y++;
//    }
//    else {
//        ppu->v.fine_y = 0;
//
//        if (ppu->v.coarse_y == 29) {
//            ppu->v.coarse_y = 0;
//            ppu->v.nametable_y ^= 1;  // Switch vertical nametable
//        }
//        else if (ppu->v.coarse_y == 31) {
//            ppu->v.coarse_y = 0;
//            // Don't switch nametable (out of bounds)
//        }
//        else {
//            ppu->v.coarse_y++;
//        }
//    }
//}
//
//// Copy horizontal bits from t to v (only when rendering is enabled)
//void ppu_copy_x(PPURegisters* ppu) {
//    // v: ....F.. ...EDCBA <- t: ....F.. ...EDCBA
//    ppu->v.nametable_x = ppu->t.nametable_x;
//    ppu->v.coarse_x = ppu->t.coarse_x;
//}
//
//// Copy vertical bits from t to v (only when rendering is enabled)
//void ppu_copy_y(PPURegisters* ppu) {
//    // v: IHGF.ED CBA..... <- t: IHGF.ED CBA.....
//    ppu->v.fine_y = ppu->t.fine_y;
//    ppu->v.nametable_y = ppu->t.nametable_y;
//    ppu->v.coarse_y = ppu->t.coarse_y;
//}
//
//// Get current VRAM address
//uint16_t ppu_get_vram_addr(PPURegisters* ppu) {
//    return *(uint16_t*)&ppu->v & 0x3FFF;  // 14-bit address
//}
//
//// Increment VRAM address (after PPUDATA read/write)
//void ppu_increment_vram_addr(PPURegisters* ppu, uint8_t increment) {
//    uint16_t* v_ptr = (uint16_t*)&ppu->v;
//    *v_ptr = (*v_ptr + increment) & 0x7FFF;
//}