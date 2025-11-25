#pragma once
#include <stdint.h>
#include <utility>
#include "Renderer.h"
#include <array>

class Core;
class NesPPU;
class Bus;

class RendererSlow : public Renderer
{
public:
	RendererSlow();
	void Initialize(NesPPU* ppu);
	void SetPPUCTRL(uint8_t value) {}
	uint8_t GetScrollX() { return m_scrollX; }
	uint8_t GetScrollY() { return m_scrollY; }
	void SetScrollX(uint8_t value);
	void SetScrollY(uint8_t value);
	//std::pair<uint8_t, uint8_t> GetPPUScroll() const { return std::make_pair(m_scrollX, m_scrollY); }
	const std::array<uint32_t, 256 * 240>& get_back_buffer() const { return m_backBuffer; }
	void reset() {
		m_backBuffer.fill(0xFFFF0000);
	}// Clear back buffer to opaque black }
	void clock();
	bool isFrameComplete() { return m_frameComplete; }
	void setFrameComplete(bool complete) { m_frameComplete = complete; }
	void SetPPUMask(uint8_t value) { }
	void SetPPUAddrHigh(uint8_t value) {}
	void SetPPUAddrLow(uint8_t value) {}
	void SetVramIncrementRight(bool val) {}

private:
	// Sprite data for current scanline
	struct Sprite {
		uint8_t x;
		uint8_t y;
		uint8_t tileIndex;
		uint8_t attributes;
		bool isSprite0;
	};
	void RenderScanline();
	void EvaluateSprites(int screenY, std::array<Sprite, 8>& newOam);
	uint8_t m_scrollX = 0;
	uint8_t m_scrollY = 0;
	int m_scanline = 0; // Current PPU scanline (0-261)
	int m_cycle = 0; // Current PPU cycle (0-340)
	bool m_frameComplete = false;
	//void render_chr_rom();
	//void render_tile(int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors);
	Bus* bus;
	NesPPU* ppu;
	std::array<uint32_t, 256 * 240> m_backBuffer;
	// Overflow can only be set once per frame
	bool hasOverflowBeenSet = false;
	bool hasSprite0HitBeenSet = false;
	std::array<Sprite, 8> secondaryOAM{};
	void get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex);
};