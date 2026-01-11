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
#include "PPUViewer.h"
#include "DebuggerContext.h"

Core::Core() : emulator(context), debuggerUI(HINST_THISCOMPONENT, *this, io), hexViewer(this, context), _dbgCtx(context.debugger_context) {
	_bus = emulator.GetBus();
}

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* nesTexture = nullptr;

bool Core::init()
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

    ppuViewer.Initialize(this, &context);

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
    init();
    emulator.start();
    return S_OK;
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
    
    float ticksPerSec = (float)SDL_GetPerformanceFrequency();
	uint64_t nextFrameTime = SDL_GetPerformanceCounter() + (uint64_t)(ticksPerSec);
    int frameCount = 0;
    while (!done) {


        // TODO IMPLEMENT THESE!
        //case ID_FILE_SAVESTATE:
        //{
        //    CommandQueue::Command cmd;
        //    cmd.type = CommandQueue::CommandType::SAVE_STATE;
        //    pMain->context.command_queue.Push(cmd);
        //    break;
        //}
        //case ID_FILE_LOADSTATE:
        //{
        //    CommandQueue::Command cmd;
        //    cmd.type = CommandQueue::CommandType::LOAD_STATE;
        //    pMain->context.command_queue.Push(cmd);
        //    break;
        //}
        //case ID_FILE_EXIT:
        //    PostQuitMessage(0);
        //    break;
        //case ID_FILE_CLOSE:
        //{
        //    CommandQueue::Command cmd;
        //    cmd.type = CommandQueue::CommandType::CLOSE;
        //    pMain->context.command_queue.Push(cmd);
        //    pMain->isPlaying = false;
        //    //InvalidateRect(hwnd, NULL, FALSE);
        //    pMain->updateMenu();
        //    //pMain->emulator.stop();
        //    pMain->ClearFrame();
        //}
        //break;
        //case ID_NES_POWER:
        //{
        //    CommandQueue::Command cmd;
        //    cmd.type = CommandQueue::CommandType::RESET;
        //    pMain->context.command_queue.Push(cmd);
        //    pMain->isPlaying = true;
        //}


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
                case SDLK_F5: {
                    _dbgCtx->is_paused.store(false);
                } break;
                case SDLK_F10: {
                    _dbgCtx->step_requested.store(true);
                    Sleep(10);
                    debuggerUI.GoTo(_dbgCtx->lastState.pc);
                } break;
                case SDLK_g: {
                    if (SDL_GetModState() & KMOD_CTRL) {
                        debuggerUI.OpenGoToAddressDialog();
					}
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
                    if (ImGui::MenuItem("Reset")) {
                        CommandQueue::Command cmd;
                        cmd.type = CommandQueue::CommandType::RESET;
                        context.command_queue.Push(cmd);
                        isPlaying = true;
                    }
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
            ImGui::Text("A: %02X", _dbgCtx->lastState.a); ImGui::SameLine();
            ImGui::Text("X: %02X", _dbgCtx->lastState.x); ImGui::SameLine();
            ImGui::Text("Y: %02X", _dbgCtx->lastState.y);
            ImGui::Separator();
            ImGui::Text("PC: %04X", _dbgCtx->lastState.pc);
            ImGui::Text("SP: %02X" , _dbgCtx->lastState.sp);

			ImGui::Separator();
			ImGui::Text("PPU Flags:");
			ImGui::Text("Dot: %d", _dbgCtx->ppuState.dot);
            ImGui::Text("Scanline: %d", _dbgCtx->ppuState.scanline);

            if (_dbgCtx->hit_breakpoint.load(std::memory_order_relaxed)) {
                _dbgCtx->hit_breakpoint.store(false);
                debuggerUI.GoTo();
			}

			bool isDebugPause = _dbgCtx->is_paused.load(std::memory_order_relaxed);
            //if (ImGui::Button(is_running ? "Pause" : "Resume")) is_running = !is_running;
            if (ImGui::Button(isDebugPause ? "Resume" : "Pause")) {
                bool newPause = !isDebugPause;
                if (newPause) {
                    _dbgCtx->is_paused.store(true);
                    _dbgCtx->continue_requested.store(false);
					debuggerUI.ComputeDisplayMap();
                }
                else {
                    _dbgCtx->continue_requested.store(true);
                }
            }
            ImGui::End();
            debuggerUI.DrawScrollableDisassembler();
            hexViewer.DrawMemoryViewer("Memory Viewer"); // 64KB of addressable memory

            if (isPlaying && !isPaused) {
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
                        titleUpdateDelay = 60;
                    }
                }
            }
            else {
                // If paused, don't burn CPU! 
                // Let the OS have the thread for a few milliseconds.
                SDL_Delay(16);
            }

            //if (Update) {
            //    Update();
            //}

            //if (ppuOpen) {
            //    // 1. Generate the pixels from PPU VRAM into a temporary buffer
            //    // You can use the logic from your old Win32 'renderNametable' here
            //    auto nt0_pixels = ppu.DebugRenderNametable(0);
            //    auto nt1_pixels = ppu.DebugRenderNametable(1);

            //    // 2. Send those pixels to the GPU
            //    ppuViewer.UpdateTexture(0, nt0_pixels);
            //    ppuViewer.UpdateTexture(1, nt1_pixels);
            //}
			ppuViewer.Draw("PPU Viewer", &ppuOpen);

            // NES Display Window
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowPos(ImVec2(300, 30), ImGuiCond_FirstUseEver);
            ImGui::Begin("Game View");
            ImGui::Text("FPS: %d", (int)context.current_fps.load(std::memory_order_relaxed));
            // Display the texture (casting the GLuint to a void*)
            // We use viewportPanelSize to make the game scale with the window
            DrawGameCentered();
            ImGui::End();
        }

		frameCount++;
        uint64_t currentTick = SDL_GetPerformanceCounter();
        if (currentTick > nextFrameTime) {
            nextFrameTime = currentTick + (uint64_t)(ticksPerSec);
            std::string title = "BlueOrb NES Emulator - FPS (UI): " + std::to_string((int)frameCount);
            SDL_SetWindowTitle(window, title.c_str());
            frameCount = 0;
		}
  //      if (currentTick < nextFrameTime) {
  //          uint64_t waitTicks = nextFrameTime - currentTick;
  //          uint32_t waitMs = (uint32_t)((waitTicks * 1000) / ticksPerSec);
  //          if (waitMs > 0) {
  //              SDL_Delay(waitMs);
  //          }
  //          // Busy-wait for the remaining time (if any)
  //          while (SDL_GetPerformanceCounter() < nextFrameTime) {}
		//}

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		//glViewport(0, 0, 800, 600);
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