#include "Core.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include "resource.h"
#include <commdlg.h>

// Viewer state
static int g_firstLine = 0;       // index of first displayed line (0-based)
static int g_linesPerPage = 1;    // computed on resize
static HFONT g_hFont = nullptr;
static int g_lineHeight = 16;     // px, will be measured
static int g_charWidth = 8;       // px, measured
static const int BYTES_PER_LINE = 16;

Core::Core() :
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

void Core::PPURenderToBackBuffer()
{
    ppu.render_frame();
}

HRESULT Core::Initialize()
{
    // Fill example buffer with demo data (0x0000 - 0x0FF)
    //g_bufferSize = 0x300; // e.g. 768 bytes
	g_bufferSize = 0x10000; // 64KB
    //g_buffer.resize(g_bufferSize);
    //for (size_t i = 0; i < g_bufferSize; ++i) g_buffer[i] = static_cast<uint8_t>(i & 0xFF);


    // Register the window class.
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Core::MainWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(LONG_PTR);
    wcex.hInstance = HINST_THISCOMPONENT;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
    wcex.lpszClassName = L"Blue NES Emulator";

    RegisterClassEx(&wcex);

    wcex.lpfnWndProc = HexWndProc;
    wcex.lpszClassName = L"HexWindowClass";
    RegisterClassEx(&wcex);

    // Register the palette window class
    wcex.lpfnWndProc = PaletteWndProc;
    wcex.lpszClassName = L"PaletteWindowClass";
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
        20,
        20,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        HINST_THISCOMPONENT,
        this);

    if (!m_hwnd)
        return S_FALSE;

	HMENU hMenu = LoadMenu(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_MENU1));
	SetMenu(m_hwnd, hMenu);

    m_hwndHex = CreateWindow(
        L"HexWindowClass",
        L"Hex Editor",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        1000,
        20,
        800,
        600,
        NULL,
        NULL,
        HINST_THISCOMPONENT,
        this);

    if (!m_hwndHex)
        return S_FALSE;

    // Create the palette window
    m_hwndPalette = CreateWindow(
        L"PaletteWindowClass",
        L"Palette Viewer",
        WS_OVERLAPPEDWINDOW,
        1000,
        620,
        400, // Width
        300, // Height
        NULL,
        NULL,
        HINST_THISCOMPONENT,
        this);

    if (!m_hwndPalette)
        return S_FALSE;

    /*bus.cart = &cart;
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    bus.core = this;*/
    bus.Initialize(this);
    ppu.bus = &bus;
    ppu.core = this;
	cpu.bus = &bus;
    ppu.set_hwnd(m_hwnd);
    HDC hdc = GetDC(m_hwnd);
    hdcMem = CreateCompatibleDC(hdc);
    hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, nullptr, nullptr, 0);
    SelectObject(hdcMem, hBitmap);
    ReleaseDC(m_hwnd, hdc);

    ShowWindow(m_hwnd, SW_SHOWNORMAL);
	ShowWindow(m_hwndHex, SW_SHOWNORMAL);
    ShowWindow(m_hwndPalette, SW_SHOWNORMAL);
    //UpdateWindow(m_hwnd);

    return S_OK;
}

LRESULT CALLBACK Core::HexWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        Core* pMain = (Core*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pMain)
        );

        result = 1;
    }
    else
    {
        Core* pMain = reinterpret_cast<Core*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (pMain)
        {
            switch (msg)
            {
            case WM_CREATE:
            {
                // Create fixed-width font (Consolas). Adjust size as desired.
                g_hFont = CreateFontW(
                    -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    ANTIALIASED_QUALITY, FF_DONTCARE | FIXED_PITCH, L"Consolas"
                );

                HDC hdc = GetDC(hwnd);
                HFONT old = (HFONT)SelectObject(hdc, g_hFont);
                TEXTMETRIC tm;
                GetTextMetrics(hdc, &tm);
                g_lineHeight = tm.tmHeight;
                // approximate char width by measuring '0'
                SIZE sz;
                GetTextExtentPoint32W(hdc, L"0", 1, &sz);
                g_charWidth = sz.cx;
                SelectObject(hdc, old);
                ReleaseDC(hwnd, hdc);

                pMain->RecalcLayout(hwnd);
                pMain->UpdateScrollInfo(hwnd);
                return 0;
            }

            case WM_SIZE:
            {
                // Resize back buffer
                HDC hdc = GetDC(hwnd);

                int newWidth = LOWORD(lParam);
                int newHeight = HIWORD(lParam);

                if (pMain->memDC)
                {
                    SelectObject(pMain->memDC, pMain->oldBitmap);
                    DeleteObject(pMain->memBitmap);
                    DeleteDC(pMain->memDC);
                }

                pMain->memDC = CreateCompatibleDC(hdc);
                pMain->memBitmap = CreateCompatibleBitmap(hdc, newWidth, newHeight);
                pMain->oldBitmap = (HBITMAP)SelectObject(pMain->memDC, pMain->memBitmap);

                pMain->bufferWidth = newWidth;
                pMain->bufferHeight = newHeight;

                ReleaseDC(hwnd, hdc);
                pMain->RecalcLayout(hwnd);
                pMain->UpdateScrollInfo(hwnd);
                //InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }
            case WM_VSCROLL:
            {
                int action = LOWORD(wParam);
                int pos = HIWORD(wParam);
                int maxLine = static_cast<int>((pMain->g_bufferSize + BYTES_PER_LINE - 1) / BYTES_PER_LINE) - 1;

                switch (action)
                {
                case SB_LINEUP:    g_firstLine = max(0, g_firstLine - 1); break;
                case SB_LINEDOWN:  g_firstLine = min(maxLine, g_firstLine + 1); break;
                case SB_PAGEUP:    g_firstLine = max(0, g_firstLine - g_linesPerPage); break;
                case SB_PAGEDOWN:  g_firstLine = min(maxLine, g_firstLine + g_linesPerPage); break;
                case SB_THUMBTRACK: g_firstLine = pos; break;
                case SB_TOP: g_firstLine = 0; break;
                case SB_BOTTOM: g_firstLine = maxLine; break;
                }
                pMain->UpdateScrollInfo(hwnd);
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }

            case WM_MOUSEWHEEL:
            {
                // Wheel rotates in multiples of WHEEL_DELTA (120). Positive = away from user = scroll up.
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                int lines = zDelta / WHEEL_DELTA; // number of detents
                g_firstLine = max(0, g_firstLine - lines * 3); // scroll 3 lines per wheel detent
                pMain->UpdateScrollInfo(hwnd);
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }

            case WM_KEYDOWN:
            {
                int maxLine = static_cast<int>((pMain->g_bufferSize + BYTES_PER_LINE - 1) / BYTES_PER_LINE) - 1;
                switch (wParam)
                {
                case VK_PRIOR: // Page Up
                    g_firstLine = max(0, g_firstLine - g_linesPerPage); break;
                case VK_NEXT: // Page Down
                    g_firstLine = min(maxLine, g_firstLine + g_linesPerPage); break;
                case VK_HOME:
                    g_firstLine = 0; break;
                case VK_END:
                    g_firstLine = maxLine; break;
                }
                pMain->UpdateScrollInfo(hwnd);
                InvalidateRect(hwnd, nullptr, TRUE);
                return 0;
            }

            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);

                if (!pMain->memDC)
                {
                    // Create initial buffer if not yet created
                    RECT client;
                    GetClientRect(hwnd, &client);
                    SendMessage(hwnd, WM_SIZE, 0, MAKELPARAM(client.right, client.bottom));
                }

                HBRUSH bgBrush = (HBRUSH)(COLOR_WINDOW + 1);
                FillRect(pMain->memDC, &ps.rcPaint, bgBrush);

                if (pMain->isPlaying) {
                    SelectObject(pMain->memDC, pMain->hFont);
                    SetBkMode(pMain->memDC, TRANSPARENT);

                    RECT rc = ps.rcPaint;
                    pMain->DrawHexDump(pMain->memDC, rc);
                }

                BitBlt(hdc,
                    ps.rcPaint.left, ps.rcPaint.top,
                    ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top,
                    pMain->memDC,
                    ps.rcPaint.left, ps.rcPaint.top,
                    SRCCOPY);

                EndPaint(hwnd, &ps);
                return 0;
            }
            case WM_ERASEBKGND:
                // Prevent flicker — we’ll handle full redraws in WM_PAINT
                return 1;

            case WM_DESTROY:
                if (pMain->memDC)
                {
                    SelectObject(pMain->memDC, pMain->oldBitmap);
                    DeleteObject(pMain->memBitmap);
                    DeleteDC(pMain->memDC);
                }
                if (pMain->hFont) DeleteObject(pMain->hFont);
                //PostQuitMessage(0);
				ShowWindow(hwnd, SW_HIDE);
                return 0;
            }

        }

        if (!wasHandled)
        {
            result = DefWindowProc(hwnd, msg, wParam, lParam);
        }
    }

    return result;
}

// Compute lines per page based on client height
void Core::RecalcLayout(HWND hwnd)
{
    RECT rc;
    GetClientRect(hwnd, &rc);
    int height = rc.bottom - rc.top;
    if (g_lineHeight > 0)
        g_linesPerPage = max(1, height / g_lineHeight);
    else
        g_linesPerPage = 1;
}

// Update vertical scrollbar to reflect buffer size
void Core::UpdateScrollInfo(HWND hwnd)
{
    int totalLines = static_cast<int>((g_bufferSize + BYTES_PER_LINE - 1) / BYTES_PER_LINE);
    int maxLine = max(0, totalLines - 1);

    g_firstLine = max(0, min(g_firstLine, maxLine));

    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = maxLine;
    si.nPage = g_linesPerPage;
    si.nPos = g_firstLine;

    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

// Draw the visible range of hex dump into rc
void Core::DrawHexDump(HDC hdc, RECT const& rc)
{
    // measure column positions (characters)
    // Layout: "ADDR: " (8 chars for 6 hex digits + ': ') -> hex bytes (3 chars each incl space) -> two spaces -> ASCII 16 chars
    // We'll position using pixel offsets computed from g_charWidth.
    int addrChars = 6; // e.g. "0000FF"
    int addrField = addrChars + 2; // "0000FF: "
    int hexCharsPerByte = 3; // "FF "
    int hexFieldChars = BYTES_PER_LINE * hexCharsPerByte;
    int gapChars = 2;
    int asciiFieldChars = BYTES_PER_LINE;

    int xAddr = 8; // margin
    int xHex = xAddr + addrField * g_charWidth;
    int xAscii = xHex + hexFieldChars * g_charWidth + gapChars * g_charWidth;
    int y = rc.top;

    // Compute which lines to draw
    int totalLines = static_cast<int>((g_bufferSize + BYTES_PER_LINE - 1) / BYTES_PER_LINE);
    int startLine = g_firstLine;
    int endLine = min(totalLines - 1, g_firstLine + g_linesPerPage - 1);

    wchar_t lineBuf[256];

    for (int line = startLine; line <= endLine; ++line)
    {
        size_t base = static_cast<size_t>(line) * BYTES_PER_LINE;
        int yLine = y + (line - startLine) * g_lineHeight;

        // Address
        swprintf_s(lineBuf, L"%06X: ", static_cast<unsigned int>(base));
        TextOutW(hdc, xAddr, yLine, lineBuf, static_cast<int>(wcslen(lineBuf)));

        // Hex bytes
        std::wstring hexs;
        hexs.reserve(BYTES_PER_LINE * 3);
        for (int b = 0; b < BYTES_PER_LINE; ++b)
        {
            if (base + b < g_bufferSize)
            {
                wchar_t tmp[8];
                if (base + b > 0x2000) {
                    int i = 0;
                }
                //swprintf_s(tmp, L"%02X ", bus.read(base + b));
                swprintf_s(tmp, L"%02X ", ppu.ReadVRAM(base + b));
                hexs += tmp;
            }
            else
            {
                hexs += L"   ";
            }
        }
        TextOutW(hdc, xHex, yLine, hexs.c_str(), static_cast<int>(hexs.size()));

        // ASCII
        std::wstring ascii;
        ascii.reserve(BYTES_PER_LINE);
        for (int b = 0; b < BYTES_PER_LINE; ++b)
        {
            if (base + b < g_bufferSize)
            {
                //uint8_t v = bus.read(base + b); //g_buffer[base + b];
                uint8_t v = ppu.ReadVRAM(base + b); //g_buffer[base + b];
                if (v >= 0x20 && v <= 0x7E) ascii.push_back(static_cast<wchar_t>(v));
                else ascii.push_back(L'.');
            }
            else
            {
                ascii.push_back(L' ');
            }
        }
        TextOutW(hdc, xAscii, yLine, ascii.c_str(), static_cast<int>(ascii.size()));
    }
}

bool ShowOpenDialog(HWND hwnd, std::wstring& filePath)
{
    OPENFILENAME ofn;
    wchar_t szFile[MAX_PATH] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(szFile[0]);

    // File filters (each filter is "Name\0Pattern\0")
    ofn.lpstrFilter = L"All Files\0*.*\0NES ROMs\0*.nes\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileName(&ofn))
    {
        MessageBox(hwnd, szFile, L"You selected:", MB_OK);
        filePath = szFile;
		return true;

        // Example: pass file path to your emulator
        // LoadRom(szFile);
    }
    return false;
}

LRESULT CALLBACK Core::MainWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (message == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        Core* pMain = (Core*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pMain)
        );

        result = 1;
    }
    else
    {
        Core* pMain = reinterpret_cast<Core*>(static_cast<LONG_PTR>(
            ::GetWindowLongPtrW(
                hwnd,
                GWLP_USERDATA
            )));

        bool wasHandled = false;

        if (pMain)
        {
            switch (message)
            {
            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                case ID_FILE_OPEN:
                {
                    std::wstring filePath;
                    if (ShowOpenDialog(hwnd, filePath))
                    {
                        pMain->LoadGame(filePath);
                    }
                    break;
                }
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
					break;
                }
                break;
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
                pMain->DrawToWindow(hdc);
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

void Core::LoadGame(const std::wstring& filePath)
{
    cart.LoadROM(filePath);
    cpu.PowerOn();
    isPlaying = true;
}

LRESULT CALLBACK Core::PaletteWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    if (msg == WM_CREATE)
    {
        LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
        Core* pMain = (Core*)pcs->lpCreateParams;

        ::SetWindowLongPtrW(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(pMain)
        );

        result = 1;
    }
    else
    {
        Core* pMain = reinterpret_cast<Core*>(static_cast<LONG_PTR>(
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
                pMain->DrawPalette(hwnd, hdc);
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

// Function to convert std::string (UTF-8) to std::wstring (UTF-16/UTF-32 depending on platform)
std::wstring stringToWstring(const std::string& str) {
    // Using UTF-8 to wide string conversion
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

// Function to convert uint32_t to hex string with zero-padding
std::string uint32ToHex(uint32_t value) {
    std::ostringstream oss;
    oss << std::hex              // use hexadecimal format
        << std::uppercase        // make letters uppercase (A-F)
        << std::setw(8)          // width of 8 characters
        << std::setfill('0')     // pad with '0'
        << value;
    return oss.str();
}

COLORREF ToColorRef(uint32_t argb)
{
    BYTE r = (argb >> 16) & 0xFF;
    BYTE g = (argb >> 8) & 0xFF;
    BYTE b = (argb) & 0xFF;
    return RGB(r, g, b); // RGB macro gives 0x00BBGGRR format
}

void Core::DrawPalette(HWND wnd, HDC hdc)
{
    RECT clientRect;
    GetClientRect(wnd, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    int cols = 4; // Number of colors per row
    int rows = 8; // Total rows (32 colors / 4 per row)
    int cellWidth = width / cols;
    int cellHeight = height / rows;

    for (int i = 0; i < 32; ++i)
    {
        int x = (i % cols) * cellWidth;
        int y = (i / cols) * cellHeight;

        uint8_t paletteIndex = ppu.paletteTable[i];
        uint32_t color = m_nesPalette[paletteIndex & 0x3F];
        //uint32_t color = m_nesPalette[i];
        // Debugging: Log palette index and color
        OutputDebugString((L"Palette Index: " + std::to_wstring(paletteIndex) + L", Color: " + stringToWstring(uint32ToHex(color)) + L"\n").c_str());

        OutputDebugString((L"x: " + std::to_wstring(x) + L", y: " + std::to_wstring(y) + L", w: " + std::to_wstring(cellWidth) + L", h: " + std::to_wstring(cellHeight) + L", Color: " + stringToWstring(uint32ToHex(color)) + L"\n").c_str());
        HBRUSH brush = CreateSolidBrush(ToColorRef(color));
        RECT rect = { x, y, x + cellWidth, y + cellHeight };
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
    }
}

HWND Core::GetWindowHandle()
{
	return m_hwnd;
}

bool Core::DrawToWindow(HDC hdc)
{
    // Copy back buffer to bitmap
    SetDIBits(hdcMem, hBitmap, 0, 240, ppu.get_back_buffer().data(), &bmi, DIB_RGB_COLORS);
    // Stretch blit to window (3x scale)
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    StretchBlt(hdc, 0, 0, clientRect.right, clientRect.bottom,
        hdcMem, 0, 0, 256, 240, SRCCOPY);
    //ReleaseDC(m_hwnd, hdc);
    return true;
}

void Core::OnResize(UINT width, UINT height)
{

}

void Core::RunMessageLoop()
{
    MSG msg;
    bool running = true;
    // --- FPS tracking variables ---
    LARGE_INTEGER frequency, lastTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    double targetFrameTime = 1.0 / 60.0;
    double accumulator = 0.0;
	double elapsedTime = 0.0;
    int frameCount = 0;
    int nextCycleCount = 30000;

    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (!running) {
            break;
        }
        if (!isPlaying) {
            // Sleep(100); // Sleep to reduce CPU usage when not playing
			continue;
        }
        // 60 FPS cap
        // --- Frame timing ---
        QueryPerformanceCounter(&currentTime);
        double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
        lastTime = currentTime;
        accumulator += deltaTime;

		// Don't run faster than 60 FPS
        if (accumulator >= targetFrameTime) {
            if (Update) {
                Update();
            }
            accumulator -= targetFrameTime;
            frameCount++;

            // Run PPU until frame complete (89342 cycles per frame)
            while (!ppu.m_frameComplete) {
                ppu.Clock();
				// CPU runs at 1/3 the speed of the PPU
				cpuCycleDebt++;

                if (cpuCycleDebt >= ppuCyclesPerCPUCycle) {
					cpuCycleDebt -= ppuCyclesPerCPUCycle;
                    cpu.Clock();
                }
                //cpu.Clock();

                // Check for NMI
				// We are removing the PPU NMI request and just triggering
				// an NMI every 30,000 CPU cycles for testing purposes
    //            if (cpu.GetCycleCount() >= nextCycleCount) {
    //                cpu.NMI();
				//	nextCycleCount += 30000;
    //                // TODO Remove this when the PPU gets integrated.
    //                break;
				//}
                //if (ppu.NMI() && !cpu.nmiRequested) {
                //    cpu.NMI();
                //}
			}
            ppu.m_frameComplete = false;
            cpu.nmiRequested = false;

			HDC hdc = GetDC(m_hwnd);
            DrawToWindow(hdc);
            // --- FPS calculation every second ---

            elapsedTime += targetFrameTime;
            if (elapsedTime >= 1.0) {
				// Updates the hex window
				// TODO - Make this more efficient by only updating changed areas
				// and also in real time rather than once per second
				InvalidateRect(m_hwndHex, nullptr, TRUE);
                double fps = frameCount / elapsedTime;
                std::wstring title = L"BlueOrb NES Emulator - FPS: " + std::to_wstring((int)fps);
                SetWindowText(m_hwnd, title.c_str());
                frameCount = 0;
                elapsedTime = 0.0;
            }
        }
        else {
			Sleep(0); // Sleep to yield CPU
        }
    }
}