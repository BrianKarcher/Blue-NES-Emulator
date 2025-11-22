#include "PPUViewer.h"
#include <Windows.h>
#include "Bus.h"
#include "Core.h"

bool PPUViewer::Initialize(HINSTANCE hInstance, Core* core) {
    this->core = core;

    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 256;
    bmi.bmiHeader.biHeight = -240; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    // Register the PPU Viewer window class
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = PPUWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = hInstance;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
    wcex.lpszClassName = L"PPUWindowClass";
    RegisterClassEx(&wcex);

    m_hwndPPUViewer = CreateWindow(
        L"PPUWindowClass",
        L"PPU Viewer",
        WS_OVERLAPPEDWINDOW,
        2000,
        20,
        800,
        600,
        NULL,
        NULL,
        hInstance,
        this);

    if (!m_hwndPPUViewer)
        return false;

    HDC hdc = GetDC(m_hwndPPUViewer);
    hdcPPUMem = CreateCompatibleDC(hdc);
    hPPUBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
    SelectObject(hdcPPUMem, hPPUBitmap);
    ReleaseDC(m_hwndPPUViewer, hdc);

    ShowWindow(m_hwndPPUViewer, SW_SHOWNORMAL);

    return true;
}

LRESULT CALLBACK PPUViewer::PPUWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        PPUViewer* pMain = (PPUViewer*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pMain)
        );

        result = 1;
    }
    else
    {
        PPUViewer* pMain = reinterpret_cast<PPUViewer*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        if (pMain)
        {
            switch (msg)
            {
            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                pMain->DrawNametables(hdc);
                EndPaint(hwnd, &ps);
                return 0;
            }

            case WM_DESTROY:
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
        }

        result = DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return result;
}

void PPUViewer::DrawNametables() {
    HDC hdcPPU = GetDC(m_hwndPPUViewer);
    DrawNametables(hdcPPU);
    ReleaseDC(m_hwndPPUViewer, hdcPPU);
}

// TODO Improve speed with dirty rectangles
// Also, cache the two nametables and only change them when needed (bank switch or write).
void PPUViewer::DrawNametables(HDC hdcPPU)
{
    if (!core->isPlaying) {
        return;
    }
    //renderNametable(nt0, 0);
    renderNametable(nt1, 1);

    //nt1.fill(m_nesPalette[0x25]);
    // Copy back buffer to bitmap
    SetDIBits(hdcPPUMem, hPPUBitmap, 0, 240, nt1.data(), &bmi, DIB_RGB_COLORS);
    //SetDIBits(hdcPPUMem, hPPUBitmap, 120, 240, nt1.data(), &bmi, DIB_RGB_COLORS);
    // Stretch blit to window (3x scale)
    RECT clientRect;
    GetClientRect(m_hwndPPUViewer, &clientRect);
    StretchBlt(hdcPPU, 0, 0, clientRect.right, clientRect.bottom,
        hdcPPUMem, 0, 0, 256, 240, SRCCOPY);
}

void PPUViewer::renderNametable(std::array<uint32_t, 256 * 240>& buffer, int physicalTable)
{
    // Clear back buffer
    // m_backBuffer.fill(0xFF000000);

    // Nametable starts at 0x2000 in VRAM
    // TODO : Support multiple nametables and mirroring
    //const uint16_t nametableAddr = 0x2000;
    const uint16_t nametableAddr = physicalTable * 0x400;

    // TODO: For now, we just render the nametable directly without scrolling or attribute tables
    std::array<uint32_t, 4> palette;
    // Render the 32x30 tile nametable
    for (int row = 0; row < 30; row++) {
        for (int col = 0; col < 32; col++) {
            // Get the tile index from the nametable in VRAM
            uint8_t tileIndex = core->ppu.m_vram[nametableAddr + row * 32 + col];

            // Calculate pixel coordinates
            int pixelX = col * 8;  // Each tile is 8x8 pixels
            int pixelY = row * 8;

            int attrRow = row / 4;
            int attrCol = col / 4;
            // Get attribute byte for the tile
            uint16_t attrAddr = (nametableAddr | 0x3c0) + attrRow * 8 + attrCol;
            uint8_t attributeByte = core->ppu.m_vram[attrAddr];
            if (attributeByte > 0) {
                int i = 0;
            }

            uint8_t paletteIndex = 0;
            get_palette_index_from_attribute(attributeByte, row, col, paletteIndex);
            core->ppu.get_palette(paletteIndex, palette);
            // Render the tile at the calculated position
            render_tile(buffer, pixelY, pixelX, tileIndex, palette);
        }
    }
}

void PPUViewer::render_tile(std::array<uint32_t, 256 * 240>& buffer,
    int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors) {
    int tileOffset = tileIndex * 16; // 16 bytes per tile

    for (int y = 0; y < 8; y++) {
        // Determine the pattern table base address
        uint16_t patternTableBase = core->ppu.GetBackgroundPatternTableBase();
        int tileBase = patternTableBase + tileOffset; // 16 bytes per tile

        uint8_t byte1 = core->bus.cart->ReadCHR(tileBase + y);     // bitplane 0
        uint8_t byte2 = core->bus.cart->ReadCHR(tileBase + y + 8); // bitplane 1

        for (int x = 0; x < 8; x++) {
            uint8_t bit0 = (byte1 >> (7 - x)) & 1;
            uint8_t bit1 = (byte2 >> (7 - x)) & 1;
            uint8_t colorIndex = (bit1 << 1) | bit0;

            uint32_t actualColor = 0;
            if (colorIndex == 0) {
                actualColor = m_nesPalette[core->ppu.paletteTable[0]]; // Transparent color (background color)
            }
            else {
                actualColor = colors[colorIndex]; // Map to actual color from palette
            }
            int renderX = pc + x;
            int renderY = pr + y;
            if (renderX < 0 || renderY < 0 || renderX >= 256 || renderY >= 240)
                continue;
            buffer[(renderY * 256) + renderX] = actualColor;
        }
    }
}

void PPUViewer::get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex)
{
    // Each attribute byte covers a 4x4 tile area (32x32 pixels)
    // Determine which quadrant of the attribute byte the tile is in
    int quadrantRow = (tileRow % 4) / 2; // 0 or 1
    int quadrantCol = (tileCol % 4) / 2; // 0 or 1
    // Extract the corresponding 2 bits for the palette index
    switch (quadrantRow) {
    case 0:
        switch (quadrantCol) {
        case 0:
            paletteIndex = attributeByte & 0x03; // Bits 0-1
            break;
        case 1:
            paletteIndex = (attributeByte >> 2) & 0x03; // Bits 2-3
            break;
        }
        break;
    case 1:
        switch (quadrantCol) {
        case 0:
            paletteIndex = (attributeByte >> 4) & 0x03; // Bits 4-5
            break;
        case 1:
            paletteIndex = (attributeByte >> 6) & 0x03; // Bits 6-7
            break;
        }
        break;
    }
}