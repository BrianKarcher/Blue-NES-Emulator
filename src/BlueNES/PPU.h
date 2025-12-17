#pragma once
#include <Windows.h>
#include <stdint.h>
#include <array>
#include <utility>
#include "SharedContext.h"
#include "MemoryMapper.h"

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
#define PPUCTRL_SPRITESIZE 0x20
#define PPUCTRL_INCREMENT 0x04
#define PPUSTATUS_VBLANK 0x80
#define PPUSTATUS_SPRITE0_HIT 0x40
#define PPUSTATUS_SPRITE_OVERFLOW 0x20
#define PPUMASK_BACKGROUNDENABLED 0x08
#define PPUMASK_SPRITEENABLED 0x10
#define PPUMASK_RENDERINGEITHER PPUMASK_BACKGROUNDENABLED | PPUMASK_SPRITEENABLED

//#define PPUDEBUG

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
class RendererLoopy;
class A12Mapper;

class PPU : public MemoryMapper
{
public:
	PPU(SharedContext& ctx);
	~PPU();
	void initialize();
	void connectBus(Bus* bus) { this->bus = bus; }
	RendererLoopy* renderer;
	void reset();
	void step();
	inline uint8_t read(uint16_t address);
	inline void write(uint16_t address, uint8_t value);
	inline void write_register(uint16_t addr, uint8_t value);
	inline uint8_t read_register(uint16_t addr);
	uint8_t ReadVRAM(uint16_t addr);
	uint8_t GetScrollX() const;
	uint8_t GetScrollY() const;

	void setMapper(A12Mapper* mapper) {
		m_mapper = mapper;
	}
	
	//void render_frame();
	
	void set_hwnd(HWND hwnd);
	// For testing, may create a window and render CHR-ROM data
	//void render_chr_rom();
	//void render_tile(int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors);

	//void OAMDMA(uint8_t* cpuMemory, uint16_t page);
	std::array<uint8_t, 0x100> oam; // 256 bytes OAM (sprite memory)
	uint8_t oamAddr;
	Bus* bus;
	A12Mapper* m_mapper;

	void Clock();
	
	std::array<uint8_t, 32> paletteTable; // 32 bytes palette table
	uint16_t GetVRAMAddress() const;
	void SetVRAMAddress(uint16_t addr);
	uint8_t GetPPUStatus() const { return m_ppuStatus; }
	uint8_t GetPPUCtrl() const { return m_ppuCtrl; }
	
	uint8_t m_ppuMask = 0;
	uint8_t m_ppuStatus = 0;
	uint8_t m_ppuCtrl = 0;
	std::array<uint8_t, 0x800> m_vram; // 2 KB VRAM
	void get_palette(uint8_t paletteIndex, std::array<uint32_t, 4>& colors);
	uint8_t get_tile_pixel_color_index(uint8_t tileIndex, uint8_t pixelInTileX, uint8_t pixelInTileY, bool isSprite, bool isSecondSprite);
	bool isFrameComplete();
	void setFrameComplete(bool complete);
	void SetPPUStatus(uint8_t flag);
	uint16_t GetBackgroundPatternTableBase() const {
		return (m_ppuCtrl & 0x10) ? 0x1000 : 0x0000; // Bit 4 of PPUCTRL
	};
	uint16_t GetSpritePatternTableBase(uint8_t tileId) const {
		if (!(m_ppuCtrl & PPUCTRL_SPRITESIZE)) {
			return (m_ppuCtrl & 0x08) ? 0x1000 : 0x0000; // Bit 3 of PPUCTRL
		}
		else {
			return (tileId & 1) == 1 ? 0x1000 : 0x000;
		}
	}
	void setBuffer(uint32_t* buf) { buffer = buf; }

private:
	SharedContext& context;
	uint32_t* buffer;
	bool is_failure = false;

	// register v in hardware PPU
	//uint16_t vramAddr = 0; // Current VRAM address (15 bits)
	// register t in hardware PPU
	//uint16_t tempVramAddr = 0; // Temporary VRAM address (15 bits)
	uint8_t ppuDataBuffer = 0; // Internal buffer for PPUDATA reads
	//int scrollY; // Fine Y scrolling (0-239)

	void write_vram(uint16_t addr, uint8_t value);
	
	//void render_nametable();
	inline void dbg(const wchar_t* fmt, ...);
	void clearBuffer(uint32_t* buffer);
};