#pragma once
#include <stdint.h>
#include <utility>
#include "Renderer.h"
#include <array>

class Core;
class NesPPU;
class Bus;

#define INTERNAL_NAMETABLE 0b000110000000000
#define INTERNAL_NAMETABLE_X 0b000010000000000
#define INTERNAL_NAMETABLE_Y 0b000100000000000
#define INTERNAL_COARSE_X 0b000000000011111
#define INTERNAL_COARSE_Y 0b000001111100000
#define INTERNAL_FINE_Y 0b111000000000000
#define INTERNAL_VERTICALBITS 0b111101111100000

class RendererWithReg : public Renderer
{
public:
	RendererWithReg();
	void Initialize(NesPPU* ppu);
	void SetPPUCTRL(uint8_t value);
	void SetScrollX(uint8_t value);
	void SetScrollY(uint8_t value);
	void SetPPUAddrHigh(uint8_t value);
	void SetPPUAddrLow(uint8_t value);
	uint16_t GetPPUAddr();
	void PPUDataAccess();
	void SetPPUMask(uint8_t value) { ppumask = value; }
	uint8_t GetScrollX();
	uint8_t GetScrollY();
	const std::array<uint32_t, 256 * 240>& get_back_buffer() const { return m_backBuffer; }
	void reset() {
		m_backBuffer.fill(0xFFFF0000);
		v = 0;
		t = 0;
		w = false;
		ppumask = 0;
	}// Clear back buffer to opaque black }
	void clock();
	bool isFrameComplete() { return m_frameComplete; }
	void setFrameComplete(bool complete) { m_frameComplete = complete; }

	//    // Internal registers
    // v and t are described as follows:
    // yyy NN YYYYY XXXXX
    // ||| || ||||| ++++ + --coarse X scroll
    // ||| || ++++ + --------coarse Y scroll
    // ||| ++--------------nametable select
    // ++ + ---------------- - fine Y scroll
    // Loopy registers
    uint16_t v = 0;   // current VRAM address (15 bits)
    uint16_t t = 0;   // temporary VRAM address (15 bits)
    uint8_t x = 0;    // fine X (0..7)
	bool w = false; // Write toggle
	void incrementX();
	void incrementY();
	// Determine whether we increment to the right, or down.
	void SetVramIncrementRight(bool val) { vramIncrementRight = val; }
	void SetWriteToggle(bool toggle);
	bool GetWriteToggle() const;
private:
	// Sprite data for current scanline
	struct Sprite {
		uint8_t x;
		uint8_t y;
		uint8_t tileIndex;
		uint8_t attributes;
		bool isSprite0;
	};
	bool vramIncrementRight = true;
	void RenderScanline();
	void EvaluateSprites(int screenY, std::array<Sprite, 8>& newOam);
	int m_scanline = 0; // Current PPU scanline (0-261)
	int m_cycle = 0; // Current PPU cycle (0-340)
	bool m_frameComplete = false;
	//void render_chr_rom();
	//void render_tile(int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors);
	Bus* bus;
	NesPPU* ppu;
	std::array<uint32_t, 256 * 240> m_backBuffer;
	uint8_t ppumask = 0;
	// Overflow can only be set once per frame
	bool hasOverflowBeenSet = false;
	bool hasSprite0HitBeenSet = false;
	std::array<Sprite, 8> secondaryOAM{};
	void get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex);
	//    // Internal helpers
	inline bool renderingEnabled() const { return (ppumask & 0x18) != 0; } // bg or sprites
	inline bool bgEnabled() const { return (ppumask & 0x08) != 0; }
	inline bool spriteEnabled() const { return (ppumask & 0x10) != 0; }
	void copyHorizontalBitsFromTtoV();
	void copyVerticalBitsFromTtoV();
};