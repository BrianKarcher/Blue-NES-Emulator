#pragma once
#include <stdint.h>
#include <array>

class NesPPU;

#define DOTS_PER_SCANLINE 340
#define SCANLINES_PER_FRAME 261

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
    void clock();
    bool isFrameComplete() { return m_frameComplete; }
    void setFrameComplete(bool complete) { m_frameComplete = complete; }

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
    bool hasSprite0HitBeenSet = false;
    bool m_frameComplete = false;
    void setPPUMask(uint8_t value) { ppumask = value; }
    uint8_t ppumask = 0;
    int dot = 0;
    std::array<Sprite, 8> secondaryOAM{};

    // Tile info
    uint8_t attributeByte;
    std::array<uint32_t, 4> palette;
    // CHR-ROM/RAM data for tile
    uint8_t chrLowByte;
    uint8_t chrHighByte;

    void evaluateSprites(int screenY, std::array<Sprite, 8>& newOam);

    // Internal helpers
    inline bool renderingEnabled() const { return (ppumask & 0x18) != 0; } // bg or sprites
    inline bool bgEnabled() const { return (ppumask & 0x08) != 0; }
    inline bool spriteEnabled() const { return (ppumask & 0x10) != 0; }
};