//#pragma once
//#include <Windows.h>
//#include <cstdint>
//#include <array>
//#include <cstring> // for memset
//
//class CPU; // forward
//class Bus;
//class Core;
//
//// NES color palette (64 colors)
////static constexpr uint32_t m_nesPalette[64] = {
////    0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
////    0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
////    0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
////    0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
////    0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
////    0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
////    0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
////    0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000
////};
////
////#define NMI_ENABLE 0x80
////
////#define NAMETABLE_WIDTH 32
////#define NAMETABLE_HEIGHT 30
////#define TILE_SIZE 8
////#define PPUCTRL 0x2000
////#define PPUMASK 0x2001
////#define PPUSTATUS 0x2002
////#define OAMADDR 0x2003
////#define OAMDATA 0x2004
////#define PPUSCROLL 0x2005
////#define PPUADDR 0x2006
////#define PPUDATA 0x2007
////#define PPUSTATUS_VBLANK 0x80
////#define PPUSTATUS_SPRITE0_HIT 0x40
////#define PPUSTATUS_SPRITE_OVERFLOW 0x20
////#define PPUMASK_BACKGROUNDENABLED 0x08
////#define PPUMASK_SPRITEENABLED 0x10
////#define PPUMASK_RENDERINGEITHER PPUMASK_BACKGROUNDENABLED | PPUMASK_SPRITEENABLED
////
////#define INTERNAL_NAMETABLE 0b000110000000000
////#define INTERNAL_NAMETABLE_X 0b000010000000000
////#define INTERNAL_NAMETABLE_Y 0b000100000000000
////#define INTERNAL_COARSE_X 0b000000000011111
////#define INTERNAL_COARSE_Y 0b000001111100000
////#define INTERNAL_FINE_Y 0b111000000000000
////#define INTERNAL_VERTICALBITS 0b111101111100000
//
//class PPU {
//public:
//    PPU();
//
//    // call this once per PPU cycle (PPU runs ~3x the CPU)
//    void tick();
//    void write_register(uint16_t addr, uint8_t value);
//	uint8_t read_register(uint16_t addr);
//    void set_hwnd(HWND hwnd);
//    uint8_t ReadVRAM(uint16_t addr);
//    uint8_t oamAddr;
//    Bus* bus;
//	Core* core;
//    bool m_frameComplete = false;
//    uint16_t GetVRAMAddress() const { return v & 0xFFFF; }
//    //std::pair<uint8_t, uint8_t> GetPPUScroll() const { return std::make_pair(m_scrollX, m_scrollY); }
//
//
//    // CPU-visible registers (simplified)
//    uint8_t ppuctrl = 0;    // $2000
//    uint8_t ppumask = 0;    // $2001
//    uint8_t ppustatus = 0;  // $2002 (read-only for CPU)
//    uint8_t oamaddr = 0;    // $2003
//    // $2004, $2005, $2006, $2007 handlers handled elsewhere
//
//    // Memory stubs (provide your actual pattern tables / palettes)
//    std::array<uint8_t, 0x800> m_vram{};    // 2 KB nametable RAM
//    std::array<uint8_t, 0x100> patternTable0{}; // stub
//    std::array<uint8_t, 0x100> patternTable1{}; // stub
//    std::array<uint8_t, 0x20> paletteTable{};
//    std::array<uint8_t, 256> OAM{}; // primary OAM (64 sprites * 4 bytes)
//
//    // Optional: screen buffer (width 256 x 240)
//    std::array<uint32_t, 256 * 240> framebuffer{}; // ARGB or index; fill as you like
//
//    // Expose state for debugging
//    int getScanline() const { return scanline; }
//    int getDot() const { return dot; }
//
//private:
//    CPU* cpu;
//    void write_vram(uint16_t addr, uint8_t value);
//    
//    uint8_t ppuDataBuffer = 0; // Internal buffer for PPUDATA reads
//

//
//    uint8_t m_ppuCtrl = 0;
//	uint8_t m_ppuMask = 0;
//
//    // Timing
//    int scanline = 261; // pre-render start
//    int dot = 0;        // 0..340
//    static constexpr int DOTS_PER_SCANLINE = 341;
//    static constexpr int SCANLINES_PER_FRAME = 262;
//
//    // NMI state
//    bool nmi_line = false;
//    bool nmi_occurred = false;
//
//    // Background pipeline registers / latches
//    uint8_t next_nt_byte = 0;
//    uint8_t next_attrib_byte = 0;
//    uint8_t next_pattern_low = 0;
//    uint8_t next_pattern_high = 0;
//
//    uint16_t bg_shift_pattern_low = 0;
//    uint16_t bg_shift_pattern_high = 0;
//    uint16_t bg_shift_attrib_low = 0;
//    uint16_t bg_shift_attrib_high = 0;
//
//    // Sprite pipeline (simplified)
//    struct SpriteEntry { uint8_t y, tile, attr, x; };
//    std::array<SpriteEntry, 8> secondaryOAM{}; // 8 sprites for current scanline
//    bool sprite0Hit = false;
//    bool spriteOverflow = false;
//
//    // Internal helpers
//    inline bool renderingEnabled() const { return (ppumask & 0x18) != 0; } // bg or sprites
//    inline bool bgEnabled() const { return (ppumask & 0x08) != 0; }
//    inline bool spriteEnabled() const { return (ppumask & 0x10) != 0; }
//
//    // Loopy helpers
//    void incrementX(); // coarse X increment (handles nametable horizontal switch)
//    void incrementY(); // fine Y / coarse Y increment (handles nametable vertical switch)
//    void copyHorizontalBitsFromTtoV(); // dot 257
//    void copyVerticalBitsFromTtoV();   // dots 280..304 pre-render
//
//    // Background pipeline
//    void loadBackgroundShifters();
//    void shiftBackgroundRegisters(); // shift by 1 on every pixel when rendering
//    void fetchBackgroundCycle();     // implements the 8-cycle fetch
//
//    // Sprite evaluation (high level; implement full timing if you need exact quirks)
//    void evaluateSpritesForNextScanline();
//
//    // Pixel rendering
//    void renderPixel();
//
//    // VBlank / NMI
//    void setVBlank();
//    void clearVBlank();
//
//    // Memory interface (must be implemented to access pattern tables / nametables / palettes)
//    uint8_t ppuRead(uint16_t addr);  // implement mapping to pattern tables, nametables, palettes
//    void ppuWrite(uint16_t addr, uint8_t value); // used for DMA in some cases
//
//    // Utility
//    inline uint16_t ntAddress() const { return 0x2000 | (v & 0x0FFF); }
//    inline uint8_t fineY() const { return (v >> 12) & 0x7; }
//};