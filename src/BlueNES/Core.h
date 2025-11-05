#pragma once
#include <Windows.h>
// C RunTime Header Files:
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <array>
#include "bus.h"
#include "nes_ppu.h"
#include "CPU.h"

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

class Core
{
public:
	Core();
	// Register the window class and call methods for instantiating drawing resources
	HRESULT Initialize();
	// Load the game ROM
	void LoadGame(
		const std::string& szFileName
	);

	HWND GetWindowHandle();
	// Process and dispatch messages
	void RunMessageLoop();
	Bus bus;
	NesPPU ppu;
	Cartridge cart;
	Processor_6502 cpu;
	// Declare a function pointer
	void (*Update)();
	// Hex dump window
	// Example buffer: replace these with your emulator memory pointer and size.
	std::vector<uint8_t> g_buffer;
	size_t g_bufferSize;
	void UpdateScrollInfo(HWND hwnd);
	void RecalcLayout(HWND hwnd);
	void DrawHexDump(HDC hdc, RECT const& rc);
private:
	int cpuCycleDebt = 0;
	int ppuCyclesPerCPUCycle = 3;
	// Draw content.
	bool DrawToWindow();
	void PPURenderToBackBuffer();
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

	// Resize the render target.
	void OnResize(
		UINT width,
		UINT height
	);
	HDC hdcMem;
	BITMAPINFO bmi;
	HBITMAP hBitmap;
	HWND m_hwnd;
	HWND m_hwndHex;
};