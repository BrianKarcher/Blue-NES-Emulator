// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "main.h"

Main::Main() :
    m_hwnd(NULL),
	hdcMem(NULL),
	hBitmap(NULL)
{
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 256;
    bmi.bmiHeader.biHeight = -240; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}

Main::~Main()
{

}

void Main::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

HRESULT Main::Initialize()
{
    // Register the window class.
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Main::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = HINST_THISCOMPONENT;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
    wcex.lpszClassName = L"Blue NES Emulator";
    
    RegisterClassEx(&wcex);

    // In terms of using the correct DPI, to create a window at a specific size
    // like this, the procedure is to first create the window hidden. Then we get
    // the actual DPI from the HWND (which will be assigned by whichever monitor
    // the window is created on). Then we use SetWindowPos to resize it to the
    // correct DPI-scaled size, then we use ShowWindow to show it.
    RECT rect = { 0, 0, 256 * 3, 240 * 3 }; // 3x scale
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindow(
        L"Blue NES Emulator",
        L"Blue NES Emulator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        HINST_THISCOMPONENT,
        this);
    
    if (!m_hwnd)
        return S_FALSE;

	ppu.set_hwnd(m_hwnd);
	HDC hdc = GetDC(m_hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
	SelectObject(hdcMem, hBitmap);
	ReleaseDC(m_hwnd, hdc);

    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    //UpdateWindow(m_hwnd);

    return S_OK;
}

LRESULT CALLBACK Main::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        Main* pMain = (Main*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pMain)
            );

        result = 1;
    }
    else
    {
        Main* pMain = reinterpret_cast<Main*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (pMain)
        {
            switch (message)
            {
                case WM_SIZE:
                    {
                        UINT width = LOWORD(lParam);
                        UINT height = HIWORD(lParam);
                        pMain->OnResize(width, height);
                    }
                    result = 0;
                    wasHandled = true;
                    break;

                case WM_DISPLAYCHANGE:
                    {
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                    result = 0;
                    wasHandled = true;
                    break;

                case WM_PAINT:
                    {
                        PAINTSTRUCT ps;
                        HDC hdc = BeginPaint(hwnd, &ps);
                        pMain->OnRender();
                        EndPaint(hwnd, &ps);
                        ValidateRect(hwnd, NULL);
                    }
                    result = 0;
                    wasHandled = true;
                    break;

                case WM_DESTROY:
                    {
                        PostQuitMessage(0);
                    }
                    result = 1;
                    wasHandled = true;
                    break;
            }
        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, message, wParam, lParam);
        }
    }

    return result;
}

bool Main::OnRender()
{
    HDC hdc = GetDC(m_hwnd);
    // Copy back buffer to bitmap
	SetDIBits(hdcMem, hBitmap, 0, 240, ppu.get_back_buffer().data(), &bmi, DIB_RGB_COLORS);
    // Stretch blit to window (3x scale)
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    StretchBlt(hdc, 0, 0, clientRect.right, clientRect.bottom,
        hdcMem, 0, 0, 256, 240, SRCCOPY);
	ReleaseDC(m_hwnd, hdc);
	return true;
}

void Main::OnResize(UINT width, UINT height)
{

}

int WINAPI WinMain(
    HINSTANCE /* hInstance */,
    HINSTANCE /* hPrevInstance */,
    LPSTR /* lpCmdLine */,
    int /* nCmdShow */
)
{
    // Use HeapSetInformation to specify that the process should
    // terminate if the heap manager detects an error in any heap used
    // by the process.
    // The return value is ignored, because we want to continue running in the
    // unlikely event that HeapSetInformation fails.
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    if (SUCCEEDED(CoInitialize(NULL)))
    {
        {
            Main main;

            if (main.Initialize() == S_OK)
            {
				main.SetupTestData();
                main.RunMessageLoop();
            }
        }
        CoUninitialize();
    }

    return 0;
}

// TODO: Move this to the test project
void Main::SetupTestData()
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
    ppu.set_chr_rom(buffer, bytesRead);
	//delete[] buffer;

	ppu.write_register(0x2006, 0x20); // PPUADDR high byte
	ppu.write_register(0x2006, 0x00); // PPUADDR low byte
    for (int r = 0; r < 15; r++) {
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board[r * 16 + c];
            ppu.write_register(0x2007, m_snakeMetatiles.TopLeft[tileIndex]);
            ppu.write_register(0x2007, m_snakeMetatiles.TopRight[tileIndex]);
		}
        for (int c = 0; c < 16; c++) {
            int tileIndex = m_board[r * 16 + c];
            ppu.write_register(0x2007, m_snakeMetatiles.BottomLeft[tileIndex]);
            ppu.write_register(0x2007, m_snakeMetatiles.BottomRight[tileIndex]);
        }
    }

    ppu.write_register(0x2006, 0x23); // PPUADDR high byte
    ppu.write_register(0x2006, 0xc0); // PPUADDR low byte
	// Generate attribute bytes for the nametable
    for (int r = 0; r < 15; r+= 2) {
        for (int c = 0; c < 16; c+= 2) {
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
            ppu.write_register(0x2007, attributeByte);
        }
    }

	// Load a simple palette (background + 4 colors)
	ppu.write_register(0x2006, 0x3F); // PPUADDR high byte
	ppu.write_register(0x2006, 0x00); // PPUADDR low byte
    for (int i = 0; i < palette.size(); i++) {
        ppu.write_register(0x2007, palette[i]);
	}
    ppu.render_frame();
}