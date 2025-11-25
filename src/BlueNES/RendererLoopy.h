#pragma once
#include <stdint.h>
#include <array>

class NesPPU;

class RendererLoopy
{
public:
    void initialize(NesPPU* ppu);
    void reset();
    void setPPUCTRL(uint8_t value);
    void writeScroll(uint8_t value);
    void ppuWriteAddr(uint8_t value);
    void ppuReadStatus();
    void ppuIncrementX();
    void ppuIncrementY();
    void ppuCopyX();
    void ppuCopyY();
    uint16_t ppuGetVramAddr();
    void ppuIncrementVramAddr(uint8_t increment);

private:
    // Sprite data for current scanline
    typedef struct Sprite {
        uint8_t x;
        uint8_t y;
        uint8_t tileIndex;
        uint8_t attributes;
        bool isSprite0;
    } Sprite;

    int m_scanline = 0; // Current PPU scanline (0-261)

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

    PPURegisters loopy;
    NesPPU* m_ppu;
    // Overflow can only be set once per frame
    bool hasOverflowBeenSet = false;

    void evaluateSprites(int screenY, std::array<Sprite, 8>& newOam);
};