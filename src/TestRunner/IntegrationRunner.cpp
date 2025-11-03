//#include "pch.h"
#include <iostream>
#include "IntegrationRunner.h"

#include <Windows.h>
#include "..\BlueNES\Core.h"
#include <chrono>

void Update();

Core core;
Bus& bus = core.bus;
NesPPU& ppu = core.ppu;
IntegrationRunner integrationRunner;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    if (core.Initialize() == S_OK)
    {
        integrationRunner.SetupTestData();
		core.Update = &Update;
        core.RunMessageLoop();
    }

    // With the following code:
    
    //const wchar_t CLASS_NAME[] = L"EmulatorTestRunner";
    //WNDCLASS wc = {};
    //wc.lpfnWndProc = WndProc;
    //wc.hInstance = hInstance;
    //wc.lpszClassName = CLASS_NAME;

    //RegisterClass(&wc);

    //HWND hwnd = CreateWindowEx(
    //    0,
    //    CLASS_NAME,
    //    L"NES Emulator Integration Test",
    //    WS_OVERLAPPEDWINDOW,
    //    CW_USEDEFAULT, CW_USEDEFAULT, 800, 720,
    //    NULL, NULL, hInstance, NULL
    //);

    //if (!hwnd) return 0;

    //ShowWindow(hwnd, nCmdShow);
    //UpdateWindow(hwnd);

    //// -------------------------------
    //// Create and initialize emulator
    //// -------------------------------
    //Main emulator(hwnd);
    //emulator.LoadCartridge("roms/nestest.nes");

    //// Run loop for N frames (e.g. 5 seconds)
    //const int maxFrames = 300;
    //int frameCount = 0;

    //MSG msg = {};
    //auto lastTime = std::chrono::high_resolution_clock::now();

    //while (msg.message != WM_QUIT && frameCount < maxFrames)
    //{
    //    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    //    {
    //        TranslateMessage(&msg);
    //        DispatchMessage(&msg);
    //    }

    //    // Run one frame of emulator
    //    emulator.Update();
    //    emulator.Render();

    //    frameCount++;

    //    // Simple FPS limiter (60Hz)
    //    auto now = std::chrono::high_resolution_clock::now();
    //    std::chrono::duration<float> elapsed = now - lastTime;
    //    if (elapsed.count() < (1.0f / 60.0f))
    //    {
    //        Sleep(static_cast<DWORD>(((1.0f / 60.0f) - elapsed.count()) * 1000));
    //    }
    //    lastTime = now;
    //}

    //return 0;
}

//LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//    switch (msg)
//    {
//    case WM_DESTROY:
//        PostQuitMessage(0);
//        return 0;
//    }
//    return DefWindowProc(hwnd, msg, wParam, lParam);
//}

void Update() {
    integrationRunner.Update();
}

void IntegrationRunner::Update() {
    //     if (scrollX >= 256) {
//         scrollX -= 256;
         //ppuCtrl ^= 0x01; // Switch nametable
//         ppu.write_register(PPUCTRL, ppuCtrl); // Update PPUCTRL with current nametable
//     }
    
    
}

void IntegrationRunner::SetupTestData()
{
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
    Bus& bus = core.bus;
    NesPPU& ppu = core.ppu;
    bus.cart->SetMirrorMode(Cartridge::MirrorMode::HORIZONTAL);
    bus.cart->SetCHRRom(buffer, bytesRead);
    delete[] buffer;

    bus.write(0x2006, 0x20); // PPUADDR high byte
    bus.write(0x2006, 0x00); // PPUADDR low byte
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board[r * 16 + c];
            bus.write(0x2007, m_snakeMetatiles.TopLeft[tileIndex]);
            bus.write(0x2007, m_snakeMetatiles.TopRight[tileIndex]);
        }
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board[r * 16 + c];
            bus.write(0x2007, m_snakeMetatiles.BottomLeft[tileIndex]);
            bus.write(0x2007, m_snakeMetatiles.BottomRight[tileIndex]);
        }
    }

    bus.write(0x2006, 0x28); // PPUADDR high byte
    bus.write(0x2006, 0x00); // PPUADDR low byte
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board2[r * 16 + c];
            bus.write(0x2007, m_snakeMetatiles.TopLeft[tileIndex]);
            bus.write(0x2007, m_snakeMetatiles.TopRight[tileIndex]);
        }
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board2[r * 16 + c];
            bus.write(0x2007, m_snakeMetatiles.BottomLeft[tileIndex]);
            bus.write(0x2007, m_snakeMetatiles.BottomRight[tileIndex]);
        }
    }

    bus.write(0x2006, 0x23); // PPUADDR high byte
    bus.write(0x2006, 0xc0); // PPUADDR low byte
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
            bus.write(0x2007, attributeByte);
        }
    }

    int secondNametableAddress = (bus.cart->GetMirrorMode() == Cartridge::MirrorMode::HORIZONTAL) ? 0x2800 : 0x2400;
    secondNametableAddress |= 0x3C0;
    bus.write(0x2006, (secondNametableAddress >> 8) & 0xFF); // PPUADDR high byte
    bus.write(0x2006, secondNametableAddress & 0xFF); // PPUADDR low byte
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
            bus.write(0x2007, attributeByte);
        }
    }

    // Load a simple palette (background + 4 colors)
    bus.write(0x2006, 0x3F); // PPUADDR high byte
    bus.write(0x2006, 0x00); // PPUADDR low byte
    for (int i = 0; i < palette.size(); i++) {
        bus.write(0x2007, palette[i]);
    }

    oam.fill(0xFF); // Initialize all sprites as hidden
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
    PerformDMA();
    //ppu.oam
    ppu.render_frame();
}

void IntegrationRunner::PerformDMA() {
    // Load OAM data into CPU memory space 0x200-0x2FF
    for (int i = 0; i < 0x100; i++) {
        bus.write(0x200 + i, oam[i]);
    }
    // Then DMA to PPU
    bus.performDMA(0x02);
}