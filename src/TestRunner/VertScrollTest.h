#pragma once
#include "IntegrationTest.h"
#include <array>

class Core;
class IntegrationRunner;

class VertScrollTest : public IntegrationTest {
public:
    void Setup(IntegrationRunner& runner) override;
    void Update() override;
    bool IsComplete() override { return m_frameCount >= m_maxFrames; }
    bool WasSuccessful() override { return m_success; }
    std::string GetName() const override { return "Vertical Scroll Test"; }

private:
    int m_frameCount = 0;
    const int m_maxFrames = 300;
    bool m_success = false;
    int m_scrollY = 0;
    uint8_t m_ppuCtrl = 0x00;
	uint8_t* oam = nullptr;

	// Using my snake game as a test
// Snake metatiles are stored as "struct of lists" to mimick how it is stored in the .asm code
// A metatile is a 16x16 pixel tile made up of 4 8x8 tiles
// Metatiles are useful because each 2x2 group of tiles shares the same attribute (palette) byte
	struct t_m_snakeMetatiles {
		std::array<uint8_t, 14> TopLeft = { 0x90, 0x92, 0xb0, 0xff, 0x05, 0x80, 0x02, 0x05, 0x11, 0x05, 0x80, 0x00, 0x15, 0x15 };
		std::array<uint8_t, 14>	TopRight = { 0x91, 0x93, 0xb1, 0xff, 0x05, 0x81, 0x05, 0x03, 0x81, 0x03, 0x10, 0x15, 0x15, 0x01 };
		std::array<uint8_t, 14> BottomLeft = { 0xa0, 0xa2, 0xc0, 0xff, 0x15, 0x80, 0x80, 0x01, 0x15, 0x01, 0x12, 0x81, 0xff, 0xff };
		std::array<uint8_t, 14> BottomRight = { 0xa1, 0xa3, 0xc1, 0xff, 0x15, 0x81, 0x00, 0x81, 0x13, 0x81, 0x15, 0xff, 0xff, 0x80 };
		std::array<uint8_t, 14> PaletteIndex = { 1, 2, 0, 0, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2 };
	} m_snakeMetatiles;

	// Board (16x15 tiles, each tile is 16x16 pixels)
	std::array<uint8_t, 240> m_board = { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };

	std::array<uint8_t, 240> m_board2 = { 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 1,
										0, 0, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 0,
										0, 0, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 0,
										0, 0, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 0,
										0, 0, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 0,
										0, 0, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 0,
										0, 0, 2, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0,
										0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 1, 0, 3, 0, 1, 0,
										0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 3, 0, 0, 0 };

	std::array<uint8_t, 4 * 8> palette = { 0x2a, 0x0f, 0x15, 0x30, // header; background
												0x0f, 0x2a, 0x27, 0x26, // grass
												0x0f, 0x0f, 0x3d, 0x2d, // grays
												0x0f, 0x1c, 0x27, 0x30, // snake
												0x2a, 0x1c, 0x27, 0x30, // snake; sprites
												0x0f, 0x25, 0x1a, 0x39, // snake toungue
												0x0f, 0x1c, 0x27, 0x30, // snake
												0x0f, 0x1c, 0x27, 0x30 }; // snake
};