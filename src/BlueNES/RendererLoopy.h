#pragma once
#include <stdint.h>

class RendererLoopy
{
public:
    void reset();
    void SetPPUCTRL(uint8_t value);
    void WriteScroll(uint8_t value);

private:
    // Loopy register structure (15-bit VRAM address)
    // yyy NN YYYYY XXXXX
    // ||| || ||||| +++++-- coarse X scroll
    // ||| || +++++-------- coarse Y scroll
    // ||| ++-------------- nametable select
    // +++----------------- fine Y scroll

    typedef struct {
        uint16_t coarse_x : 5;  // Coarse X scroll (0-31)
        uint16_t coarse_y : 5;  // Coarse Y scroll (0-31)
        uint16_t nametable_x : 1;  // Nametable X bit
        uint16_t nametable_y : 1;  // Nametable Y bit
        uint16_t fine_y : 3;    // Fine Y scroll (0-7)
        uint16_t unused : 1;    // Always 0
    } LoopyRegister;

    typedef struct {
        LoopyRegister v;  // Current VRAM address (15 bits)
        LoopyRegister t;  // Temporary VRAM address (15 bits) / address of top-left tile
        uint8_t x;        // Fine X scroll (3 bits)
        bool w;           // Write latch (first/second write toggle)
    } PPURegisters;

    PPURegisters ppu;
};