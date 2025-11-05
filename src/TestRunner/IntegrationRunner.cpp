//#include "pch.h"
#include <iostream>
#include "IntegrationRunner.h"

#include <Windows.h>
#include "Core.h"
#include <chrono>
#include "VertScrollTest.h"
#include "SimpleNESTest.h"

void Update();

Core core;
IntegrationRunner integrationRunner;

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    if (core.Initialize() == S_OK)
    {
        integrationRunner.Initialize(&core);
        // Register tests
        auto scrollTest = new SimpleNESTest();
		scrollTest->Setup(integrationRunner);
        integrationRunner.RegisterTest(scrollTest);

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

// IntegrationRunner implementation
IntegrationRunner::IntegrationRunner() {
    oam.fill(0xFF);
}

void IntegrationRunner::Initialize(Core* core) {
    m_core = core;
}

void IntegrationRunner::RegisterTest(IntegrationTest* test) {
    m_test = test;
}

void IntegrationRunner::Update() {
    auto currentTest = m_test;
    currentTest->Update();

    if (currentTest->IsComplete()) {
        std::cout << "Test '" << currentTest->GetName() << "' completed. ";
        std::cout << (currentTest->WasSuccessful() ? "SUCCESS" : "FAILED") << std::endl;
        m_currentTest++;
    }
}

void IntegrationRunner::PerformDMA() {
    // Load OAM data into CPU memory space 0x200-0x2FF
    for (int i = 0; i < 0x100; i++) {
        m_core->bus.write(0x200 + i, oam[i]);
    }
    // Then DMA to PPU
    m_core->bus.performDMA(0x02);
}

void IntegrationRunner::RunTests()
{

}

uint8_t* IntegrationRunner::LoadFile(const char* filename, size_t& bytesRead) {
    // read CHR-ROM data from file and load into PPU for testing
    // read from file
    FILE* file = nullptr;
    errno_t err = fopen_s(&file, filename, "rb");
    if (err != 0 || !file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return nullptr;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (fileSize <= 0) {
        std::cerr << "File is empty or error reading size: " << filename << std::endl;
        fclose(file);
        return nullptr;
    }
    uint8_t* buffer = new uint8_t[fileSize];
    bytesRead = fread(buffer, 1, fileSize, file);
    fclose(file);
    if (bytesRead != fileSize) {
        std::cerr << "Error reading file: " << filename << std::endl;
        delete[] buffer;
        return nullptr;
    }
    return buffer;
}

HWND IntegrationRunner::GetWindowHandle()
{
	return m_core->GetWindowHandle();
}