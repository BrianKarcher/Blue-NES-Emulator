#pragma once
#include <Windows.h>
#include <array>

class Core;

class PPUViewer
{
public:
	bool Initialize(HINSTANCE hInstance, Core* core);
	void DrawNametables();
private:
	//void PPURenderToBackBuffer();
	
	void renderNametable(std::array<uint32_t, 256 * 240>& buffer, int physicalTable);
	void get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex);
	void render_tile(std::array<uint32_t, 256 * 240>& buffer,
		int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors);
	void DrawNametables(HDC hdcPPU);

	HDC hdcPPUMem0;
	HDC hdcPPUMem1;
	HWND m_hwndPPUViewer;
	HBITMAP hPPUBitmap0;
	HBITMAP hPPUBitmap1;
	BITMAPINFO bmi;
	std::array<uint32_t, 256 * 240> nt0;
	std::array<uint32_t, 256 * 240> nt1;
	static LRESULT CALLBACK PPUWndProc(
		HWND hwnd,
		UINT msg,
		WPARAM wParam,
		LPARAM lParam
	);
	Core* core;
};