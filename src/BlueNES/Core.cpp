#include "Core.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include "resource.h"
#include <commdlg.h>
#include "AudioBackend.h"
#include "SharedContext.h"
#include "DebuggerUI.h"
#include "ImGuiFileDialog.h"

Core::Core() : emulator(context), debuggerUI(HINST_THISCOMPONENT, *this) {
	_bus = emulator.GetBus();
}

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* nesTexture = nullptr;

bool Core::init(HWND wnd)
{
    // Setup SDL
    //if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER /* | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD*/) < 0)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        SDL_Log("SDL Init Error: %s", SDL_GetError());
        return false;
    }

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    window = SDL_CreateWindow("NES Emulator - Dear ImGui", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    
    if (!window)
    {
        SDL_Log("Window Error: %s", SDL_GetError());
        return false;
    }

    gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    //SDL_GL_SetSwapInterval(1); // Enable vsync

    // 2. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    // 3. Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glGenTextures(1, &nes_texture);
    glBindTexture(GL_TEXTURE_2D, nes_texture);

    // Set texture parameters for "pixel perfect" look (Nearest neighbor)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Initialize with empty data (256x240 pixels, RGBA format)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 240, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);

 //   renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	//// Removed SDL_RENDERER_PRESENTVSYNC to allow uncapped framerate for better timing control
 //   if (!renderer)
 //   {
 //       SDL_Log("Renderer Error: %s", SDL_GetError());
 //       return false;
 //   }

 //   // Create NES framebuffer texture (256x240)
 //   nesTexture = SDL_CreateTexture(
 //       renderer,
 //       SDL_PIXELFORMAT_ARGB8888,    // NES framebuffer format
 //       SDL_TEXTUREACCESS_STREAMING, // CPU-updated texture
 //       256, 240
 //   );

 //   if (!nesTexture)
 //   {
 //       SDL_Log("Texture Error: %s", SDL_GetError());
 //       return false;
 //   }

    return true;
}

// Viewer state
static int g_firstLine = 0;       // index of first displayed line (0-based)
static int g_linesPerPage = 1;    // computed on resize
static HFONT g_hFont = nullptr;
static int g_lineHeight = 16;     // px, will be measured
static int g_charWidth = 8;       // px, measured
static const int BYTES_PER_LINE = 16;

HRESULT Core::Initialize()
{
	g_bufferSize = 0x10000; // 64KB

    return S_OK;
}

HRESULT Core::CreateWindows() {
    init(m_hwnd);

    hexSources[0] = hexReadCPU;
    hexSources[1] = hexReadPPU;

    emulator.start();
    return S_OK;
}

uint8_t hexReadPPU(Core* core, uint16_t val) {
    //return core->emulator.nes.ppu.ReadVRAM(val);
    return 0;
}

uint8_t hexReadCPU(Core* core, uint16_t val) {
    //return core->emulator.nes.bus.read(val);
    return 0;
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
					//pMain->cpu.toggleFrozen(); // Toggle freeze
                    break;
                }
                break;
			}
            case WM_COMMAND:
                switch (LOWORD(wParam))
                {
                case ID_DEBUG_STEPOVER: {
                    //pMain->debuggerUI.StepInto();
                    // F10 pressed
                    //CommandQueue::Command cmd;
                    //cmd.type = CommandQueue::CommandType::SAVE_STATE;
                    //pMain->context.command_queue.Push(cmd);
                } break;
                case ID_FILE_OPEN:
                {

                    break;
                }
                case ID_FILE_SAVESTATE:
                {
                    CommandQueue::Command cmd;
                    cmd.type = CommandQueue::CommandType::SAVE_STATE;
                    pMain->context.command_queue.Push(cmd);
                    break;
                }
                case ID_FILE_LOADSTATE:
                {
                    CommandQueue::Command cmd;
                    cmd.type = CommandQueue::CommandType::LOAD_STATE;
                    pMain->context.command_queue.Push(cmd);
                    break;
                }
                case ID_FILE_EXIT:
                    PostQuitMessage(0);
					break;
                case ID_FILE_CLOSE:
                {
                    CommandQueue::Command cmd;
                    cmd.type = CommandQueue::CommandType::CLOSE;
                    pMain->context.command_queue.Push(cmd);
                    pMain->isPlaying = false;
                    //InvalidateRect(hwnd, NULL, FALSE);
                    pMain->updateMenu();
                    //pMain->emulator.stop();
                    pMain->ClearFrame();
                }
                    break;
                case ID_NES_RESET:
                {
                    CommandQueue::Command cmd;
                    cmd.type = CommandQueue::CommandType::RESET;
                    pMain->context.command_queue.Push(cmd);
                    pMain->isPlaying = true;
                }
                    break;
                case ID_NES_POWER:
                {
                    CommandQueue::Command cmd;
                    cmd.type = CommandQueue::CommandType::RESET;
                    pMain->context.command_queue.Push(cmd);
                    pMain->isPlaying = true;
                }
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
                //pMain->RenderFrame();
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

void Core::updateMenu() {

}

void Core::LoadGame(const std::string& filePath)
{
    CommandQueue::Command cmd;
    cmd.type = CommandQueue::CommandType::LOAD_ROM;
    cmd.data = filePath;
    context.command_queue.Push(cmd);
    isPlaying = true;
	//audioBackend->AddBuffer(44100 / 60); // Pre-fill 1 frame of silence to avoid pops
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
    //RECT clientRect;
    //GetClientRect(wnd, &clientRect);

    //int width = clientRect.right - clientRect.left;
    //int height = clientRect.bottom - clientRect.top;

    //int cols = 4; // Number of colors per row
    //int rows = 8; // Total rows (32 colors / 4 per row)
    //int cellWidth = width / cols;
    //int cellHeight = height / rows;

    //for (int i = 0; i < 32; ++i)
    //{
    //    int x = (i % cols) * cellWidth;
    //    int y = (i / cols) * cellHeight;

    //    uint8_t paletteIndex = emulator.nes.ppu.paletteTable[i];
    //    uint32_t color = m_nesPalette[paletteIndex & 0x3F];
    //    HBRUSH brush = CreateSolidBrush(ToColorRef(color));
    //    RECT rect = { x, y, x + cellWidth, y + cellHeight };
    //    FillRect(hdc, &rect, brush);
    //    DeleteObject(brush);
    //}
}

bool Core::ClearFrame()
{
    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    return true;
}

bool Core::RenderFrame(const uint32_t* frame_data)
{
    // 1. Update the texture with your emulator's pixel buffer
    // Assuming 'ppu_buffer' is a uint32_t array[256 * 240] 
    glBindTexture(GL_TEXTURE_2D, nes_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, frame_data);

    // Update texture with NES framebuffer
    //SDL_UpdateTexture(
    //    nesTexture,
    //    nullptr,
    //    frame_data,
    //    256 * sizeof(uint32_t)
    //);

    //// Clear screen
    //SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    //SDL_RenderClear(renderer);

    //// Scale to window automatically
    //SDL_Rect dstRect{ 0, 0, 0, 0 };
    //SDL_GetWindowSizeInPixels(window, (int*)&dstRect.w, (int*)&dstRect.h);

    //if (SDL_RenderCopy(renderer, nesTexture, nullptr, &dstRect) != 0) {
    //    printf("SDL_RenderCopy failed: %s\n", SDL_GetError());
    //}

    //SDL_RenderPresent(renderer);

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

void Core::DrawMemoryViewer(const char* title, size_t size) {
    ImGui::Begin(title);

    // Set a default size for the viewer
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    // We use 1 column for the Address, 16 for Hex data, and 1 for ASCII
    static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("MemTable", 18, flags, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
        // Setup columns
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        for (int i = 0; i < 16; i++) ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        // Use Clipper: each row displays 16 bytes
        ImGuiListClipper clipper;
        clipper.Begin((int)((size + 15) / 16));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                ImGui::TableNextRow();

                // Column 0: Address
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%04X:", row * 16);

                // Columns 1-16: Hex Bytes
                for (int n = 0; n < 16; n++) {
                    ImGui::TableSetColumnIndex(n + 1);
                    size_t addr = row * 16 + n;
                    if (addr < size) {
                        uint8_t val = _bus->peek(addr);

                        // Highlight non-zero values or specific addresses (like Stack or Zero Page)
                        if (val == 0) ImGui::TextDisabled("00");
                        else ImGui::Text("%02X", val);
                    }
                }

                // Column 17: ASCII representation
                ImGui::TableSetColumnIndex(17);
                char ascii[17];
                for (int n = 0; n < 16; n++) {
                    size_t addr = row * 16 + n;
                    uint8_t c = (addr < size) ? _bus->peek(addr) : ' ';
                    ascii[n] = (c >= 32 && c <= 126) ? c : '.';
                }
                ascii[16] = '\0';
                ImGui::TextUnformatted(ascii);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

void Core::DrawGameCentered() {
    // Get the current window (or viewport) size
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    // NES target aspect ratio (4:3)
    float targetAR = 4.0f / 3.0f;
    float windowAR = windowSize.x / windowSize.y;

    ImVec2 displaySize;
    if (windowAR > targetAR) {
        // Window is too wide (Letterbox on sides)
        displaySize.y = windowSize.y;
        displaySize.x = windowSize.y * targetAR;
    }
    else {
        // Window is too tall (Pillarbox top/bottom)
        displaySize.x = windowSize.x;
        displaySize.y = windowSize.x / targetAR;
    }

    // Center the image in the available space
    ImVec2 pos = ImGui::GetCursorScreenPos();
    pos.x += (windowSize.x - displaySize.x) * 0.5f;
    pos.y += (windowSize.y - displaySize.y) * 0.5f;

    ImGui::SetCursorScreenPos(pos);
    ImGui::Image((void*)(intptr_t)nes_texture, displaySize);
}

void Core::RunMessageLoop()
{
    MSG msg;
    bool done = false;

    int cpuCycleDebt = 0;
    const int RENDER_TIMEOUT_MS = 16;
    static int titleUpdateDelay = 60;
    HACCEL hAccel = LoadAccelerators(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_ACCELERATOR1));
    
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
            case SDL_KEYDOWN: {
                switch (event.key.keysym.sym) {
                case SDLK_ESCAPE: {
                    CommandQueue::Command cmd;
                    bool newPauseState = !isPaused;
                    cmd.type = newPauseState ? CommandQueue::CommandType::PAUSE : CommandQueue::CommandType::RESUME;
                    context.command_queue.Push(cmd);
                    isPaused = newPauseState;
                } break;
                case SDLK_F11: {
                    // Toggle Fullscreen on Alt+Enter
                    bool isFullScreen = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
                    SDL_SetWindowFullscreen(window, isFullScreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP);
                } break;
                }
            } break;
            case SDL_QUIT:
                done = true;
                break;
            case SDL_CONTROLLERDEVICEADDED:
            {
                CommandQueue::Command cmd;
                cmd.type = CommandQueue::CommandType::ADD_CONTROLLER;
                context.command_queue.Push(cmd);
            }
            break;

            case SDL_CONTROLLERDEVICEREMOVED:
            {
                CommandQueue::Command cmd;
                cmd.type = CommandQueue::CommandType::REMOVE_CONTROLLER;
                context.command_queue.Push(cmd);
            }
            break;

            default:
                break;
            }
        }

        if (done) {
            break;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 4. EMULATOR UI
        {
            // Main Menu Bar
            if (ImGui::BeginMainMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Load ROM...")) {
						IGFD::FileDialogConfig config;
						//config.title = "Select NES ROM";
						//config.filters = { ".nes" };

                        // Arguments: Key, Title, Filter, Path
                        ImGuiFileDialog::Instance()->OpenDialog("ChooseRomKey", "Select NES ROM", ".nes", config);
                    }
                    if (ImGui::MenuItem("Exit")) done = true;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Emulation")) {
                    if (ImGui::MenuItem("Reset")) { /* Reset NES */ }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // Display the dialog
            ImVec2 minSize = ImVec2(800, 500);
            ImVec2 maxSize = ImVec2(1200, 800);
            if (ImGuiFileDialog::Instance()->Display("ChooseRomKey", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
                if (ImGuiFileDialog::Instance()->IsOk()) {
                    std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                    std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
                    LoadGame(filePathName);
                    isPaused = false;
                    updateMenu();
                    // Load the ROM into your emulator
                    // MyEmulator.Load(filePathName);
                }

                // Close the dialog
                ImGuiFileDialog::Instance()->Close();
            }

            // In a real emulator, you'd render your NES PPU buffer to an OpenGL texture
            // and display it here using ImGui::Image()
            //ImGui::Button("Mock NES Screen (256x240)", ImVec2(512, 480));
            // Get the size of the current window to scale the image
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

            // Debugger Window (CPU Registers)
            ImGui::SetNextWindowPos(ImVec2(1100, 600), ImGuiCond_FirstUseEver);
            ImGui::Begin("CPU Debugger");
            ImGui::Text("A: 0x00"); ImGui::SameLine();
            ImGui::Text("X: 0x00"); ImGui::SameLine();
            ImGui::Text("Y: 0x00");
            ImGui::Separator();
            ImGui::Text("PC: 0x8000");
            ImGui::Text("SP: 0xFD");
            //if (ImGui::Button(is_running ? "Pause" : "Resume")) is_running = !is_running;
            ImGui::Button("Pause");
            ImGui::End();
            debuggerUI.DrawScrollableDisassembler();

			DrawMemoryViewer("Memory Viewer", 0x10000); // 64KB of addressable memory

            if (!isPaused && isPlaying) {
                // Wait for the Core (The "Sleep" phase)
                // We wait up to 20ms. If the Core finishes in 5ms, we wake up in 5ms.
                // If the Core hangs, we wake up in 20ms anyway to handle SDL events again.
                const uint32_t* frame_data = context.WaitForNewFrame(20);

                //OutputDebugStringW((L"CPU Cycles: " + std::to_wstring(cpuCyclesThisFrame) +
                //    L", Audio Samples: " + std::to_wstring(audioBuffer.size()) +
                //    L", Queued: " + std::to_wstring(queuedSamples) + L"\n").c_str());
                if (frame_data) {
                    // 5. Rendering
                    RenderFrame(frame_data);
                    if (titleUpdateDelay-- <= 0) {
                        std::wstring title = L"BlueOrb NES Emulator - FPS: " + std::to_wstring((int)context.current_fps);
                        SetWindowText(m_hwnd, title.c_str());
                        titleUpdateDelay = 60;
                    }
                }
            }

            if (Update) {
                Update();
            }

            // NES Display Window
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(300, 30), ImGuiCond_FirstUseEver);
            ImGui::Begin("Game View");
            ImGui::Text("FPS: %.1f", io.Framerate);
            // Display the texture (casting the GLuint to a void*)
            // We use viewportPanelSize to make the game scale with the window
            DrawGameCentered();
            ImGui::End();
        }

        ImGui::Render();
        //glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glViewport(0, 0, 800, 600);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);

        //if (timeSinceStart >= 0.25) {
        //    //ppuViewer.DrawNametables();

        //    // Updates the hex window
        //    // TODO - Make this more efficient by only updating changed areas
        //    // and also in real time rather than once per second
        //    InvalidateRect(hHexDrawArea, nullptr, TRUE);
        //    double fps = frameCount / timeSinceStart;
        //    frameStartTime = currentTime;
        //    frameCount = 0;
        //}
    }
    emulator.stop();
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}