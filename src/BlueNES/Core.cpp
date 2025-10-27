#include "Core.h"

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

void Core::Render()
{
    ppu.render_frame();
    OnRender();
}

HRESULT Core::Initialize()
{
    // Register the window class.
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = Core::WndProc;
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

    bus.cart = &cart;
    bus.cpu = &cpu;
    bus.ppu = &ppu;
    ppu.bus = &bus;
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

LRESULT CALLBACK Core::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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

bool Core::OnRender()
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

void Core::OnResize(UINT width, UINT height)
{

}

void Core::RunMessageLoop()
{
    MSG msg;
    bool running = true;
    // --- FPS tracking variables ---
    LARGE_INTEGER freq, now, last;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    double targetFrameTime = 1.0 / 60.0;
    double elapsedTime = 0.0;
    int frameCount = 0;

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
        if (Update) {
			Update();
        }

        Render();

        // 60 FPS cap
        // --- Frame timing ---
        QueryPerformanceCounter(&now);
        double frameTime = (now.QuadPart - last.QuadPart) / (double)freq.QuadPart;

        // Busy-wait (or Sleep) until 16.67 ms have passed
        while (frameTime < targetFrameTime) {
            Sleep(0); // yields CPU
            QueryPerformanceCounter(&now);
            frameTime = (now.QuadPart - last.QuadPart) / (double)freq.QuadPart;
        }

        last = now;

        // --- FPS calculation every second ---
        frameCount++;
        elapsedTime += frameTime;
        if (elapsedTime >= 1.0) {
            double fps = frameCount / elapsedTime;
            std::wstring title = L"BlueOrb NES Emulator - FPS: " + std::to_wstring((int)fps);
            SetWindowText(m_hwnd, title.c_str());
            frameCount = 0;
            elapsedTime = 0.0;
        }
    }
}