#pragma once

#include <Windows.h>
// C RunTime Header Files:
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <array>
#include "bus.h"
#include "nes_ppu.h"
#include "processor_6502.h"

//#include <d2d1.h>
//#include <d2d1helper.h>
//#include <dwrite.h>
//#include <wincodec.h>

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

class Main
{
public:
	Main();
	~Main();

	// Register the window class and call methods for instantiating drawing resources
	HRESULT Initialize();

	// Process and dispatch messages
	void RunMessageLoop();

	void SetupTestData();

private:
	Bus bus;
	NesPPU ppu;
	Processor_6502 cpu;
	HDC hdcMem;
	BITMAPINFO bmi;
	HBITMAP hBitmap;
	// Draw content.
	bool OnRender();
	void Render();

	// Resize the render target.
	void OnResize(
		UINT width,
		UINT height
	);

	// The windows procedure.
	static LRESULT CALLBACK WndProc(
		HWND hWnd,
		UINT message,
		WPARAM wParam,
		LPARAM lParam
	);

	HWND m_hwnd;
	// Using my snake game as a test
	// Snake metatiles are stored as "struct of lists" to mimick how it is stored in the .asm code
	// A metatile is a 16x16 pixel tile made up of 4 8x8 tiles
	// Metatiles are useful because each 2x2 group of tiles shares the same attribute (palette) byte
	struct t_m_snakeMetatiles {
		std::array<uint8_t, 14> TopLeft = { 0x90, 0x92, 0xb0, 0xff, 0x05, 0x80, 0x02, 0x05, 0x11, 0x05, 0x80, 0x00, 0x15, 0x15 };
		std::array<uint8_t, 14>	TopRight = { 0x91, 0x93, 0xb1, 0xff, 0x05, 0x81, 0x05, 0x03, 0x81, 0x03, 0x10, 0x15, 0x15, 0x01 };
		std::array<uint8_t, 14> BottomLeft = { 0xa0, 0xa2, 0xc0, 0xff, 0x15, 0x80, 0x80, 0x01, 0x15, 0x01, 0x12, 0x81, 0xff, 0xff };
		std::array<uint8_t, 14> BottomRight = { 0xa1, 0xa3, 0xc1, 0xff, 0x15, 0x81, 0x00, 0x81, 0x13, 0x81, 0x15, 0xff, 0xff, 0x80 };
		std::array<uint8_t, 14> PaletteIndex = { 1, 2, 0, 0, 3, 3, 3, 3, 3, 3, 3, 2, 2, 2 };
	} m_snakeMetatiles;

	// Board (16x15 tiles, each tile is 16x16 pixels)
	std::array<uint8_t, 240> m_board = {	0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0,
											0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0,
											0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 };
	std::array<uint8_t, 4 * 8> palette = {		0x2a, 0x0f, 0x15, 0x30, // header; background
												0x0f, 0x2a, 0x27, 0x26, // grass
												0x0f, 0x0f, 0x3d, 0x2d, // grays
												0x0f, 0x1c, 0x27, 0x30, // snake
												0x2a, 0x1c, 0x27, 0x30, // snake; sprites
												0x0f, 0x25, 0x1a, 0x39, // snake toungue
												0x0f, 0x1c, 0x27, 0x30, // snake
												0x0f, 0x1c, 0x27, 0x30 }; // snake
	
	//ID2D1Factory* m_pDirect2dFactory;
	//ID2D1HwndRenderTarget* m_pRenderTarget;
	//ID2D1SolidColorBrush* m_pLightSlateGrayBrush;
	//ID2D1SolidColorBrush* m_pCornflowerBlueBrush;
	//ID2D1Bitmap* m_pBitmap;
};