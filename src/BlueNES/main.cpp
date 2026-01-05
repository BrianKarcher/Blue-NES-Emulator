#include "main.h"
#include "Core.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>

//int WINAPI WinMain(
//    HINSTANCE /* hInstance */,
//    HINSTANCE /* hPrevInstance */,
//    LPSTR /* lpCmdLine */,
//    int /* nCmdShow */
//)
//{
//    // Use HeapSetInformation to specify that the process should
//    // terminate if the heap manager detects an error in any heap used
//    // by the process.
//    // The return value is ignored, because we want to continue running in the
//    // unlikely event that HeapSetInformation fails.
//    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
//
//    if (SUCCEEDED(CoInitialize(NULL)))
//    {
//        {
//            Core core;
//
//            if (core.Initialize() == S_OK && core.CreateWindows() == S_OK)
//            {
//                core.RunMessageLoop();
//            }
//        }
//        CoUninitialize();
//    }
//
//    return 0;
//}

int main(int argc, char* argv[]) {
    // 1. Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) return -1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_Window* window = SDL_CreateWindow("NES Emulator - Dear ImGui", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // 2. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

    ImGui::StyleColorsDark();

    // 3. Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    bool show_demo_window = true;
    bool done = false;

    // --- EMULATOR STATE ---
    bool is_running = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
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
                    if (ImGui::MenuItem("Load ROM...")) { /* Open file dialog */ }
                    if (ImGui::MenuItem("Exit")) done = true;
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Emulation")) {
                    if (ImGui::MenuItem("Reset")) { /* Reset NES */ }
                    ImGui::EndMenu();
                }
                ImGui::EndMainMenuBar();
            }

            // NES Display Window
            ImGui::Begin("Game View");
            ImGui::Text("FPS: %.1f", io.Framerate);
            // In a real emulator, you'd render your NES PPU buffer to an OpenGL texture
            // and display it here using ImGui::Image()
            ImGui::Button("Mock NES Screen (256x240)", ImVec2(512, 480));
            ImGui::End();

            // Debugger Window (CPU Registers)
            ImGui::Begin("CPU Debugger");
            ImGui::Text("A: 0x00"); ImGui::SameLine();
            ImGui::Text("X: 0x00"); ImGui::SameLine();
            ImGui::Text("Y: 0x00");
            ImGui::Separator();
            ImGui::Text("PC: 0x8000");
            ImGui::Text("SP: 0xFD");
            if (ImGui::Button(is_running ? "Pause" : "Resume")) is_running = !is_running;
            ImGui::End();
        }

        // 5. Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}