#pragma once
#include <Windows.h>
// C RunTime Header Files:
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <array>
#include "bus.h"
#include "PPU.h"
#include "CPU.h"
#include "APU.h"
#include "Input.h"
#include "PPUViewer.h"
#include "EmulatorCore.h"
#include "SharedContext.h"
#include "DebuggerUI.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <stdio.h>

template<class Interface>
inline void SafeRelease(
	Interface** ppInterfaceToRelease
)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = NULL;
	}
}

#ifndef Assert
#if defined( DEBUG ) || defined( _DEBUG )
#define Assert(b) do {if (!(b)) {OutputDebugStringA("Assert: " #b "\n");}} while (0)
#else
#define Assert(b)
#endif //DEBUG || _DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

uint8_t hexReadPPU(Core* core, uint16_t val);
uint8_t hexReadCPU(Core* core, uint16_t val);

class Core
{
public:
	Core();
	bool init(HWND wnd);
	SDL_GLContext gl_context;
	ImGuiIO io;
	SharedContext context;
	EmulatorCore emulator;
	PPUViewer ppuViewer;
	bool ppuOpen = true;
	
	// Register the window class and call methods for instantiating drawing resources
	HRESULT Initialize();
	HRESULT CreateWindows();
	// Load the game ROM
	void LoadGame(
		const std::string& szFileName
	);

	HWND GetWindowHandle();
	// Process and dispatch messages
	void RunMessageLoop();

	void (*Update)();
	// Hex dump window
	// Example buffer: replace these with your emulator memory pointer and size.
	uint8_t* g_buffer;
	size_t g_bufferSize;
	void UpdateScrollInfo(HWND hwnd);
	void RecalcLayout(HWND hwnd);
	void DrawHexDump(HDC hdc, RECT const& rc);
	void DrawPalette(HWND wnd, HDC hdc);
	HFONT hFont = nullptr;
	HDC memDC = nullptr;
	HBITMAP memBitmap = nullptr;
	HBITMAP oldBitmap = nullptr;
	int bufferWidth = 0, bufferHeight = 0;
	int lineHeight = 16;
	HWND m_hwndPalette;
	bool isPlaying = false;

	HWND hHexCombo = NULL;
	HWND hHexDrawArea = NULL;
	int hexView = 0;
	DebuggerUI debuggerUI;
	//PPUViewer ppuViewer;
private:
	GLuint nes_texture;
	HMENU hMenu;
	Bus* _bus;
	void updateMenu();
	bool RenderFrame(const uint32_t* frame_data);
	bool ClearFrame();
	void DrawGameCentered();

	void PollControllerState();
	bool isPaused;
	// The windows procedure.
	static LRESULT CALLBACK MainWndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);
	static LRESULT CALLBACK HexWndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);
	static LRESULT CALLBACK HexDrawAreaProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);
	static LRESULT CALLBACK PaletteWndProc(
		HWND hwnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam
	);

	// Resize the render target.
	void OnResize(
		UINT width,
		UINT height
	);
	
	HWND m_hwnd;
	// Hex Window handles
	HWND m_hwndHex;
	std::array<uint8_t(*)(Core*, uint16_t), 2> hexSources;
	void DrawMemoryViewer(const char* title, size_t size);
};