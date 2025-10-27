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
#include "processor_6502.h"

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

	// Process and dispatch messages
	void RunMessageLoop();
	Bus bus;
	NesPPU ppu;
	// Declare a function pointer
	void (*Update)();
private:
	// Draw content.
	bool DrawToWindow();
	void PPURenderToBackBuffer();
	// The windows procedure.
	static LRESULT CALLBACK WndProc(
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
	Cartridge cart;
	Processor_6502 cpu;
	HDC hdcMem;
	BITMAPINFO bmi;
	HBITMAP hBitmap;
	HWND m_hwnd;
};