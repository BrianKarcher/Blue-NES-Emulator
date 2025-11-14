#pragma once
#include <Windows.h>
#include <stdint.h>
#include <array>
#include <utility>

#define NMI_ENABLE 0x80

#define NAMETABLE_WIDTH 32
#define NAMETABLE_HEIGHT 30
#define TILE_SIZE 8
#define PPUCTRL 0x2000
#define PPUMASK 0x2001
#define PPUSTATUS 0x2002
#define OAMADDR 0x2003
#define OAMDATA 0x2004
#define PPUSCROLL 0x2005
#define PPUADDR 0x2006
#define PPUDATA 0x2007
#define PPUSTATUS_VBLANK 0x80
#define PPUSTATUS_SPRITE0_HIT 0x40
#define PPUSTATUS_SPRITE_OVERFLOW 0x20

// NES color palette (64 colors)
static constexpr uint32_t m_nesPalette[64] = {
	0xFF666666, 0xFF002A88, 0xFF1412A7, 0xFF3B00A4, 0xFF5C007E, 0xFF6E0040, 0xFF6C0600, 0xFF561D00,
	0xFF333500, 0xFF0B4800, 0xFF005200, 0xFF004F08, 0xFF00404D, 0xFF000000, 0xFF000000, 0xFF000000,
	0xFFADADAD, 0xFF155FD9, 0xFF4240FF, 0xFF7527FE, 0xFFA01ACC, 0xFFB71E7B, 0xFFB53120, 0xFF994E00,
	0xFF6B6D00, 0xFF388700, 0xFF0C9300, 0xFF008F32, 0xFF007C8D, 0xFF000000, 0xFF000000, 0xFF000000,
	0xFFFFFEFF, 0xFF64B0FF, 0xFF9290FF, 0xFFC676FF, 0xFFF36AFF, 0xFFFE6ECC, 0xFFFE8170, 0xFFEA9E22,
	0xFFBCBE00, 0xFF88D800, 0xFF5CE430, 0xFF45E082, 0xFF48CDDE, 0xFF4F4F4F, 0xFF000000, 0xFF000000,
	0xFFFFFEFF, 0xFFC0DFFF, 0xFFD3D2FF, 0xFFE8C8FF, 0xFFFBC2FF, 0xFFFEC4EA, 0xFFFECCC5, 0xFFF7D8A5,
	0xFFE4E594, 0xFFCFEF96, 0xFFBDF4AB, 0xFFB3F3CC, 0xFFB5EBF2, 0xFFB8B8B8, 0xFF000000, 0xFF000000
};

class Bus;
class Core;

class NesPPU
{
public:
	NesPPU();
	~NesPPU();
	void reset();
	void step();
	void write_register(uint16_t addr, uint8_t value);
	uint8_t read_register(uint16_t addr);
	uint8_t ReadVRAM(uint16_t addr);
	void RenderScanline();
	void render_frame();
	const std::array<uint32_t, 256 * 240>& get_back_buffer() const { return m_backBuffer; }
	void set_hwnd(HWND hwnd);
	// For testing, may create a window and render CHR-ROM data
	void render_chr_rom();
	void render_tile(int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors);

	//void OAMDMA(uint8_t* cpuMemory, uint16_t page);
	std::array<uint8_t, 0x100> oam; // 256 bytes OAM (sprite memory)
	uint8_t oamAddr;
	Bus* bus;
	Core* core;
	bool NMI();
	void Clock();
	bool m_frameComplete = false;
	std::array<uint8_t, 32> paletteTable; // 32 bytes palette table
	uint16_t GetVRAMAddress() const { return vramAddr; }
	void SetVRAMAddress(uint16_t addr) { vramAddr = addr & 0x3FFF; }
	uint8_t GetPPUStatus() const { return m_ppuStatus; }
	uint8_t GetPPUCtrl() const { return m_ppuCtrl; }
	std::pair<uint8_t, uint8_t> GetPPUScroll() const { return std::make_pair(m_scrollX, m_scrollY); }
private:
	// Sprite data for current scanline
	struct Sprite {
		uint8_t x;
		uint8_t y;
		uint8_t tileIndex;
		uint8_t attributes;
		bool isSprite0;
	};

	bool is_failure = false;
	
	void EvaluateSprites(int screenY, std::array<Sprite, 8>& newOam);

	// register v in hardware PPU
	uint16_t vramAddr = 0; // Current VRAM address (15 bits)
	// register t in hardware PPU
	uint16_t tempVramAddr = 0; // Temporary VRAM address (15 bits)
	uint8_t ppuDataBuffer = 0; // Internal buffer for PPUDATA reads

	// TODO: This can be optimized to use less memory if needed
	// Change to 0x800 (2 KB)
	std::array<uint8_t, 0x800> m_vram; // 2 KB VRAM

	// Back buffer for rendering (256x240 pixels, RGBA)
	std::array<uint32_t, 256 * 240> m_backBuffer;
	int m_cycle = 0; // Current PPU cycle (0-340)
	int m_scanline = 0; // Current PPU scanline (0-261)
	uint8_t m_scrollX = 0;
	uint8_t m_scrollY = 0;
	uint8_t m_ppuCtrl = 0;
	uint8_t m_ppuStatus = 0;
	uint16_t GetBackgroundPatternTableBase() const {
		return (m_ppuCtrl & 0x10) ? 0x1000 : 0x0000; // Bit 4 of PPUCTRL
	}

	uint16_t GetSpritePatternTableBase() const {
		return (m_ppuCtrl & 0x08) ? 0x1000 : 0x0000; // Bit 3 of PPUCTRL
	}
	//int scrollY; // Fine Y scrolling (0-239)
	
	bool writeToggle = false; // Toggle for first/second write to PPUSCROLL/PPUADDR

	void write_vram(uint16_t addr, uint8_t value);
	void get_palette(uint8_t paletteIndex, std::array<uint32_t, 4>& colors);
	void get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex);
	void render_nametable();
	uint8_t get_tile_pixel_color_index(uint8_t tileIndex, uint8_t pixelInTileX, uint8_t pixelInTileY, bool isSprite);
	
	std::array<Sprite, 8> secondaryOAM{};
	// Overflow can only be set once per frame
	bool hasOverflowBeenSet = false;
};