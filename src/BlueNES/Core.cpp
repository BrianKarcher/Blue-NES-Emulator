#include "Core.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include "resource.h"
#include <commdlg.h>
#include "AudioBackend.h"
#include <SDL.h>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* nesTexture = nullptr;

bool init_sdl(HWND wnd)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER /* | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD*/) < 0)
    {
        SDL_Log("SDL Init Error: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindowFrom((void*)wnd);

    if (!window)
    {
        SDL_Log("Window Error: %s", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
    {
        SDL_Log("Renderer Error: %s", SDL_GetError());
        return false;
    }

    // Create NES framebuffer texture (256x240)
    nesTexture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,    // NES framebuffer format
        SDL_TEXTUREACCESS_STREAMING, // CPU-updated texture
        256, 240
    );

    if (!nesTexture)
    {
        SDL_Log("Texture Error: %s", SDL_GetError());
        return false;
    }

    return true;
}

// Viewer state
static int g_firstLine = 0;       // index of first displayed line (0-based)
static int g_linesPerPage = 1;    // computed on resize
static HFONT g_hFont = nullptr;
static int g_lineHeight = 16;     // px, will be measured
static int g_charWidth = 8;       // px, measured
static const int BYTES_PER_LINE = 16;

Core::Core() {
}

//void Core::PPURenderToBackBuffer()
//{
//    ppu.render_frame();
//}

HRESULT Core::Initialize()
{
    // Fill example buffer with demo data (0x0000 - 0x0FF)
    //g_bufferSize = 0x300; // e.g. 768 bytes
	g_bufferSize = 0x10000; // 64KB
    //g_buffer.resize(g_bufferSize);
    //for (size_t i = 0; i < g_bufferSize; ++i) g_buffer[i] = static_cast<uint8_t>(i & 0xFF);


    // Register the window class.

    /*bus.cart = &cart;
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    bus.core = this;*/
    bus.Initialize(this);
    ppu.Initialize(&bus, this);
    cart.initialize(&bus);
	cpu.bus = &bus;
    //ppu.set_hwnd(m_hwnd);
    
    //UpdateWindow(m_hwnd);

	audioBackend = new AudioBackend();
    // Initialize audio backend
    if (!audioBackend->Initialize(44100, 1)) {  // 44.1kHz, mono
        // Handle error - audio failed to initialize
        MessageBox(m_hwnd, L"Failed to initialize audio!", L"Error", MB_OK);
    }

    // Set up DMC read callback
    apu.set_dmc_read_callback([this](uint16_t address) -> uint8_t {
        return bus.read(address);
    });

    // --- Debug: force a tone on Pulse 1 ---
    //apu.write_register(0x4000, 0x30); // duty + envelope: constant volume 0x0 / vol 0x0? adjust below
    //apu.write_register(0x4002, 0xFF); // timer low
    //apu.write_register(0x4003, 0x07); // timer high -> make very low frequency (adjust)
    //apu.write_register(0x4015, 0x01); // enable pulse 1

    //// Increase volume to audible: 0x10 sets constant volume, low nibble is volume (0x0 - 0xF).
    //apu.write_register(0x4000, 0x90); // 1 0 0 1 0000 -> constant volume + volume 0 (tweak to 0x9..0xF)

    return S_OK;
}

HRESULT Core::CreateWindows() {
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

    // Register draw area child window
    WNDCLASS dc = {};
    dc.lpfnWndProc = HexDrawAreaProc;
    dc.hInstance = HINST_THISCOMPONENT;
    dc.lpszClassName = L"DRAW_AREA";
    dc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&dc);

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

    init_sdl(m_hwnd);

    HMENU hMenu = LoadMenu(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_MENU1));
    SetMenu(m_hwnd, hMenu);

    m_hwndHex = CreateWindow(
        L"HexWindowClass",
        L"Hex Editor",
        WS_OVERLAPPEDWINDOW,
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

    if (!ppuViewer.Initialize(HINST_THISCOMPONENT, this)) {
        return S_FALSE;
    }

    hexSources[0] = hexReadCPU;
    hexSources[1] = hexReadPPU;

    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    ShowWindow(m_hwndHex, SW_SHOWNORMAL);
    ShowWindow(m_hwndPalette, SW_SHOWNORMAL);

    return S_OK;
}

uint8_t hexReadPPU(Core* core, uint16_t val) {
    return core->ppu.ReadVRAM(val);
}

uint8_t hexReadCPU(Core* core, uint16_t val) {
    return core->bus.read(val);
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

        pMain->hHexCombo = CreateWindowEx(
            0, L"COMBOBOX", NULL,
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            0, 0, 200, 200,
            hwnd, NULL, NULL, NULL
        );

        int index = SendMessage(pMain->hHexCombo, CB_ADDSTRING, 0, (LPARAM)L"CPU Memory");
        SendMessage(pMain->hHexCombo, CB_SETITEMDATA, index, (LPARAM)0);
        index = SendMessage(pMain->hHexCombo, CB_ADDSTRING, 0, (LPARAM)L"PPU Memory");
        SendMessage(pMain->hHexCombo, CB_SETITEMDATA, index, (LPARAM)1);
        index = SendMessage(pMain->hHexCombo, CB_SETCURSEL, 0, 0);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Child draw area with scroll bar
        pMain->hHexDrawArea = CreateWindowEx(
            0, L"DRAW_AREA", NULL,
            WS_CHILD | WS_VISIBLE | WS_VSCROLL,
            0, 20, rect.right - rect.left, rect.bottom - rect.top - 20,
            hwnd, NULL, HINST_THISCOMPONENT, pMain
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
            //case WM_PAINT:
            //{
            //    InvalidateRect(pMain->hHexDrawArea, nullptr, TRUE);
            //    return 0;
            //}
            case WM_COMMAND:
            {
            case CBN_SELCHANGE:
            {
                int sel = (int)SendMessage(pMain->hHexCombo, CB_GETCURSEL, 0, 0);
                int value = (int)SendMessage(pMain->hHexCombo, CB_GETITEMDATA, sel, 0);

                pMain->hexView = value;
                //pMain->UpdateHexView();          // redraw child window
                InvalidateRect(pMain->hHexDrawArea, NULL, TRUE);
                return 0;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
            }

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

LRESULT CALLBACK Core::HexDrawAreaProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

        if (pMain) {
            switch (msg)
            {
            case WM_CREATE:
            {
                // configure scrollbar range
                SCROLLINFO si = { sizeof(si) };
                si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
                si.nMin = 0;
                //si.nMax = bufferH;
                si.nPage = 100; // updated in resize
                si.nPos = 0;
                SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
                break;
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
    uint8_t (*fp)(Core*, uint16_t);
    fp = hexSources[hexView];

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
                swprintf_s(tmp, L"%02X ", fp(this, base + b));
                hexs += tmp;
                //swprintf_s(tmp, L"%02X ", ppu.ReadVRAM(base + b));
                //if (base + b < cart.m_chrData.size()) {
                //    swprintf_s(tmp, L"%02X ", cart.m_chrData[base + b]);
                //    hexs += tmp;
                //}
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
                uint8_t v = 0;
                v = fp(this, base + b); //g_buffer[base + b];
                //v = ppu.ReadVRAM(base + b); //g_buffer[base + b];
                //if (base + b < cart.m_chrData.size()) {
                //    v = cart.m_chrData[base + b];
                //}
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
            case WM_KEYDOWN:
            {
                switch (wParam)
                {
                case VK_F1:
                    // Show palette window
                    ShowWindow(pMain->m_hwndHex, SW_SHOWNORMAL);
                    break;
                case VK_ESCAPE:
                    pMain->isPaused = !pMain->isPaused;
					//pMain->cpu.toggleFrozen(); // Toggle freeze
                    break;
                }
                break;
			}
            case WM_KEYUP:
            {
                switch (wParam)
                {
                case 'A':
                    pMain->input.ButtonUp(BUTTON_B);
                    break;
                case 'S':
                    pMain->input.ButtonUp(BUTTON_A);
                    break;
                case 'Z':
                    pMain->input.ButtonUp(BUTTON_SELECT);
                    break;
                case 'X':
                    pMain->input.ButtonUp(BUTTON_START);
                    break;
                case VK_UP:
                    pMain->input.ButtonUp(BUTTON_UP);
                    break;
                case VK_DOWN:
                    pMain->input.ButtonUp(BUTTON_DOWN);
                    break;
                case VK_LEFT:
                    pMain->input.ButtonUp(BUTTON_LEFT);
                    break;
                case VK_RIGHT:
                    pMain->input.ButtonUp(BUTTON_RIGHT);
                    break;
                }
                break;
            }
            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                case ID_FILE_OPEN:
                {
                    std::wstring filePath;
                    if (ShowOpenDialog(hwnd, filePath))
                    {
                        pMain->LoadGame(filePath);
                        pMain->isPaused = false;
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
                pMain->DrawToWindow();
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
    cart.unload();
    cart.LoadROM(filePath);
    cpu.PowerOn();
    isPlaying = true;
	audioBackend->AddBuffer(44100 / 60); // Pre-fill 1 frame of silence to avoid pops
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
//std::wstring stringToWstring(const std::string& str) {
//    // Using UTF-8 to wide string conversion
//    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
//    return converter.from_bytes(str);
//}

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
        //OutputDebugString((L"Palette Index: " + std::to_wstring(paletteIndex) + L", Color: " + stringToWstring(uint32ToHex(color)) + L"\n").c_str());

        //OutputDebugString((L"x: " + std::to_wstring(x) + L", y: " + std::to_wstring(y) + L", w: " + std::to_wstring(cellWidth) + L", h: " + std::to_wstring(cellHeight) + L", Color: " + stringToWstring(uint32ToHex(color)) + L"\n").c_str());
        HBRUSH brush = CreateSolidBrush(ToColorRef(color));
        RECT rect = { x, y, x + cellWidth, y + cellHeight };
        FillRect(hdc, &rect, brush);
        DeleteObject(brush);
    }
}

bool Core::DrawToWindow()
{
    // Update texture with NES framebuffer
    SDL_UpdateTexture(
        nesTexture,
        nullptr,
        ppu.get_back_buffer().data(),
        256 * sizeof(uint32_t)
    );

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Scale to window automatically
    SDL_Rect dstRect{ 0, 0, 0, 0 };
    SDL_GetWindowSizeInPixels(window, (int*)&dstRect.w, (int*)&dstRect.h);

    if (SDL_RenderCopy(renderer, nesTexture, nullptr, &dstRect) != 0) {
        printf("SDL_RenderCopy failed: %s\n", SDL_GetError());
    }

    SDL_RenderPresent(renderer);

    //ppu.framebuffer.fill(0xff0f0f0f);
    // Copy back buffer to bitmap
    //SetDIBits(hdcMem, hBitmap, 0, 240, ppu.get_back_buffer().data(), &bmi, DIB_RGB_COLORS);
    //// Stretch blit to window (3x scale)
    //RECT clientRect;
    //GetClientRect(m_hwnd, &clientRect);
    //StretchBlt(hdc, 0, 0, clientRect.right, clientRect.bottom,
    //    hdcMem, 0, 0, 256, 240, SRCCOPY);
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
    LARGE_INTEGER frequency, frameStartTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&frameStartTime);
    double targetFrameTime = 1.0 / 60.0;
    double nextUpdate = 0.0;
    int frameCount = 0;

    // --- Audio setup ---
    // Audio sample rate (44.1kHz is standard)
    const int AUDIO_SAMPLE_RATE = 44100;
    const double AUDIO_SAMPLE_PERIOD = 1.0 / AUDIO_SAMPLE_RATE;
    double audioAccumulator = 0.0;

    // Audio buffer for queueing samples
    std::vector<float> audioBuffer;
    audioBuffer.reserve(4096);

    double audioFraction = 0.0;  // Per-frame fractional pos
    int cpuCycleDebt = 0;
    const int ppuCyclesPerCPUCycle = 3;
    
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

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_CONTROLLERDEVICEADDED:
                input.OpenFirstController();
                break;

            case SDL_CONTROLLERDEVICEREMOVED:
				input.CloseController();
                break;

            default:
                break;
            }
        }
        if (!running) {
            break;
        }
        if (!isPlaying) {
            continue;
        }

        if (isPaused) {
            Sleep(100); // Sleep to reduce CPU usage while paused
            continue;
        }

        if (Update) {
            Update();
        }
        input.PollControllerState();

        const double CPU_FREQ = 1789773.0;
        const double cyclesPerSample = CPU_FREQ / 44100.0;  // 40.58 exact
        const int TARGET_SAMPLES_PER_FRAME = 735; // 44100 / 60 = 735 samples per frame

        cpu.cyclesThisFrame = 0;

        // Run PPU until frame complete (89342 cycles per frame)
        int cpuCyclesThisFrame = 0;
        while (!ppu.isFrameComplete()) {
            ppu.Clock();
            // CPU runs at 1/3 the speed of the PPU
            cpuCycleDebt++;

            while (cpuCycleDebt >= ppuCyclesPerCPUCycle) {
                //cpuCycleDebt -= ppuCyclesPerCPUCycle;
                uint64_t cyclesElapsed = cpu.Clock();
                cpuCycleDebt -= ppuCyclesPerCPUCycle * cyclesElapsed;

                // Clock APU for each CPU cycle
                for (uint64_t i = 0; i < cyclesElapsed; ++i) {
                    apu.step();

                    // Generate audio sample based on cycle timing
                    audioFraction += 1.0;
                    while (audioFraction >= cyclesPerSample) {
                        audioBuffer.push_back(apu.get_output());
                        audioFraction -= cyclesPerSample;
                    }
                }
            }
        }
        //OutputDebugStringW((L"CPU Cycles this frame: " + std::to_wstring(cpu.cyclesThisFrame) + L"\n").c_str());
        cpu.nmiRequested = false;
        ppu.setFrameComplete(false);

        // Submit the exact samples generated this frame
        // Check audio queue to prevent unbounded growth
        size_t queuedSamples = audioBackend->GetQueuedSampleCount();
        const size_t MAX_QUEUED_SAMPLES = 4410; // ~100ms of audio (44100 / 10)

        if (queuedSamples < MAX_QUEUED_SAMPLES) {
            audioBackend->SubmitSamples(audioBuffer.data(), audioBuffer.size());
            // Clear audio buffer for next frame
            audioBuffer.clear();
        }
        else {
            // Audio queue is too large - skip this frame's audio to catch up
            //OutputDebugStringW(L"Audio queue overflow - dropping frame\n");
        }

        //OutputDebugStringW((L"CPU Cycles: " + std::to_wstring(cpuCyclesThisFrame) +
        //    L", Audio Samples: " + std::to_wstring(audioBuffer.size()) +
        //    L", Queued: " + std::to_wstring(queuedSamples) + L"\n").c_str());

        DrawToWindow();

        frameCount++;
        // Debug output every second
        QueryPerformanceCounter(&currentTime);
        double timeSinceStart = (currentTime.QuadPart - frameStartTime.QuadPart) / (double)frequency.QuadPart;

        if (timeSinceStart >= 0.25) {
            ppuViewer.DrawNametables();

            // Updates the hex window
            // TODO - Make this more efficient by only updating changed areas
            // and also in real time rather than once per second
            InvalidateRect(hHexDrawArea, nullptr, TRUE);
            double fps = frameCount / timeSinceStart;
            std::wstring title = L"BlueOrb NES Emulator - FPS: " + std::to_wstring((int)fps) + L" Cycle " + std::to_wstring(cpu.GetCycleCount());
            SetWindowText(m_hwnd, title.c_str());
            frameStartTime = currentTime;
            frameCount = 0;
        }

        // === FRAME PACING: Wait for correct frame time ===
        LARGE_INTEGER frameEndTime;
        QueryPerformanceCounter(&frameEndTime);

        double frameTimeElapsed = (frameEndTime.QuadPart - frameStartTime.QuadPart) / (double)frequency.QuadPart;
        double timeToWait = targetFrameTime * frameCount - frameTimeElapsed;

        if (timeToWait > 0.001) { // If more than 1ms to wait
            DWORD sleepMs = (DWORD)((timeToWait - 0.001) * 1000.0); // Sleep for most of it
            if (sleepMs > 0) {
                Sleep(sleepMs);
            }

            // Busy wait for the remaining time for accuracy
            do {
                QueryPerformanceCounter(&frameEndTime);
                frameTimeElapsed = (frameEndTime.QuadPart - frameStartTime.QuadPart) / (double)frequency.QuadPart;
            } while (frameTimeElapsed < targetFrameTime * frameCount);
        }
    }
    cart.unload();
    audioBackend->Shutdown();
    input.CloseController();
    /*for (int i = 0; i < controllers.size(); ++i) {
        SDL_GameControllerClose(controllers[i]);
        controllers[i] = nullptr;
	}*/
    SDL_Quit();
}