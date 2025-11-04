#include "VertScrollTest.h"
#include "nes_ppu.h"
#include "Core.h"
#include <iostream>
#include "IntegrationRunner.h"

Core* m_core;
Bus* bus;
NesPPU* ppu;
IntegrationRunner* m_runner;

void VertScrollTest::Setup(IntegrationRunner& runner)
{
    m_runner = &runner;
    m_core = runner.GetCore();
    oam = m_runner->oam.data();
	// Initialize test environment
	bus = &m_core->bus;
	ppu = &m_core->ppu;

	m_ppuCtrl = 0x00;
	m_scrollY = 0;
    m_success = false;

    // read CHR-ROM data from file and load into PPU for testing
    const char* filename = "test-chr-rom.chr";
    // read from file
    FILE* file = nullptr;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0 || !file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (fileSize <= 0) {
        std::cerr << "File is empty or error reading size: " << filename << std::endl;
        fclose(file);
        return;
    }
    uint8_t* buffer = new uint8_t[fileSize];
    size_t bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) {
        std::cerr << "Error reading file: " << filename << std::endl;
        delete[] buffer;
        return;
    }

    bus->cart->SetMirrorMode(Cartridge::MirrorMode::HORIZONTAL);
    bus->cart->SetCHRRom(buffer, bytesRead);
    delete[] buffer;

    bus->write(0x2006, 0x20); // PPUADDR high byte
    bus->write(0x2006, 0x00); // PPUADDR low byte
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board[r * 16 + c];
            bus->write(0x2007, m_snakeMetatiles.TopLeft[tileIndex]);
            bus->write(0x2007, m_snakeMetatiles.TopRight[tileIndex]);
        }
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board[r * 16 + c];
            bus->write(0x2007, m_snakeMetatiles.BottomLeft[tileIndex]);
            bus->write(0x2007, m_snakeMetatiles.BottomRight[tileIndex]);
        }
    }

    bus->write(0x2006, 0x28); // PPUADDR high byte
    bus->write(0x2006, 0x00); // PPUADDR low byte
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board2[r * 16 + c];
            bus->write(0x2007, m_snakeMetatiles.TopLeft[tileIndex]);
            bus->write(0x2007, m_snakeMetatiles.TopRight[tileIndex]);
        }
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board2[r * 16 + c];
            bus->write(0x2007, m_snakeMetatiles.BottomLeft[tileIndex]);
            bus->write(0x2007, m_snakeMetatiles.BottomRight[tileIndex]);
        }
    }

    bus->write(0x2006, 0x23); // PPUADDR high byte
    bus->write(0x2006, 0xc0); // PPUADDR low byte
    // Generate attribute bytes for the nametable
    for (int r = 0; r < 15; r += 2) {
        for (int c = 0; c < 16; c += 2) {
            int tileIndex = r * 16 + c;
            int topLeft = m_board[tileIndex];
            int topRight = m_board[tileIndex + 1];
            // The last row has no bottom tiles
            int bottomLeft = r == 14 ? 0 : m_board[tileIndex + 16];
            int bottomRight = r == 14 ? 0 : m_board[tileIndex + 17];
            uint8_t attributeByte = 0;
            attributeByte |= (m_snakeMetatiles.PaletteIndex[topLeft] & 0x03) << 0; // Top-left
            attributeByte |= (m_snakeMetatiles.PaletteIndex[topRight] & 0x03) << 2; // Top-right
            attributeByte |= (m_snakeMetatiles.PaletteIndex[bottomLeft] & 0x03) << 4; // Bottom-left
            attributeByte |= (m_snakeMetatiles.PaletteIndex[bottomRight] & 0x03) << 6; // Bottom-right
            bus->write(0x2007, attributeByte);
        }
    }

    int secondNametableAddress = (bus->cart->GetMirrorMode() == Cartridge::MirrorMode::HORIZONTAL) ? 0x2800 : 0x2400;
    secondNametableAddress |= 0x3C0;
    bus->write(0x2006, (secondNametableAddress >> 8) & 0xFF); // PPUADDR high byte
    bus->write(0x2006, secondNametableAddress & 0xFF); // PPUADDR low byte
    // Generate attribute bytes for the nametable
    for (int r = 0; r < 15; r += 2) {
        for (int c = 0; c < 16; c += 2) {
            int tileIndex = r * 16 + c;
            int topLeft = m_board2[tileIndex];
            int topRight = m_board2[tileIndex + 1];
            // The last row has no bottom tiles
            int bottomLeft = r == 14 ? 0 : m_board2[tileIndex + 16];
            int bottomRight = r == 14 ? 0 : m_board2[tileIndex + 17];
            uint8_t attributeByte = 0;
            attributeByte |= (m_snakeMetatiles.PaletteIndex[topLeft] & 0x03) << 0; // Top-left
            attributeByte |= (m_snakeMetatiles.PaletteIndex[topRight] & 0x03) << 2; // Top-right
            attributeByte |= (m_snakeMetatiles.PaletteIndex[bottomLeft] & 0x03) << 4; // Bottom-left
            attributeByte |= (m_snakeMetatiles.PaletteIndex[bottomRight] & 0x03) << 6; // Bottom-right
            bus->write(0x2007, attributeByte);
        }
    }

    // Load a simple palette (background + 4 colors)
    bus->write(0x2006, 0x3F); // PPUADDR high byte
    bus->write(0x2006, 0x00); // PPUADDR low byte
    for (int i = 0; i < palette.size(); i++) {
        bus->write(0x2007, palette[i]);
    }

    // Create a simple sprite for testing
    oam[0] = 100; // Y position
    oam[1] = 0x06;   // Tile index
    oam[2] = 0x40;   // Attributes
    oam[3] = 120; // X position
    oam[4] = 100; // Y position
    oam[5] = 0x07;   // Tile index
    oam[6] = 0;   // Attributes
    oam[7] = 128; // X position
    oam[8] = 108; // Y position
    oam[9] = 0x16;   // Tile index
    oam[10] = 0;   // Attributes
    oam[11] = 120; // X position
    oam[12] = 108; // Y position
    oam[13] = 0x17;   // Tile index
    oam[14] = 0;   // Attributes
    oam[15] = 128; // X position
    m_runner->PerformDMA();
    //ppu.oam
    ppu->render_frame();
}

void VertScrollTest::Update()
{
	m_scrollY++;
	if (m_scrollY >= 240) {
		m_scrollY -= 240;
		m_ppuCtrl ^= 0x02; // Switch nametable
		ppu->write_register(PPUCTRL, m_ppuCtrl); // Update PPUCTRL with current nametable
	}

	ppu->write_register(PPUSCROLL, 0x00); // No horizontal scroll
	ppu->write_register(PPUSCROLL, m_scrollY);
	//scrollX++;

	// Move sprite
	oam[3] += 1; // X position
	oam[7] += 1; // X position of 2nd sprite
	oam[11] += 1; // X position of 3rd sprite
	oam[15] += 1; // X position of 4th sprite
    m_runner->PerformDMA();
}