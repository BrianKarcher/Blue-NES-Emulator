#pragma once
#include <stdint.h>
#include <array>

class NesPPU
{
public:
	NesPPU();
	~NesPPU();
	void reset();
	void step();
	void write_register(uint16_t addr, uint8_t value);
	uint8_t read_register(uint16_t addr);
	void render_scanline();
	void render_frame();
	void set_chr_rom(uint8_t* chrData, size_t size);
	const std::array<uint32_t, 256 * 240>& get_back_buffer() const { return m_backBuffer; }
private:
	uint8_t* m_pchrRomData = nullptr;
	uint16_t vramAddr = 0; // Current VRAM address (15 bits)
	size_t m_chrRomSize = 0;
	// Back buffer for rendering (256x240 pixels, RGBA)
	std::array<uint32_t, 256 * 240> m_backBuffer;
	bool writeToggle = false; // Toggle for first/second write to PPUSCROLL/PPUADDR
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
};

