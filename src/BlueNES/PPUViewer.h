#pragma once
#include <array>
#include <SDL_opengl.h>

class Core;
class Bus;
class PPU;
class SharedContext;
class Cartridge;
class DebuggerContext;

class PPUViewer
{
public:
	bool Initialize(Core* core, SharedContext* sharedCtx);
	void Draw(const char* title, bool* open);

private:
	GLuint ntTextures[2] = { 0, 0 };
	//void PPURenderToBackBuffer();
	
	void UpdateTexture(int index, std::array<uint32_t, 256 * 240>& data);
	void renderNametable(std::array<uint32_t, 256 * 240>& buffer, int physicalTable);
	void get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex);
	void render_tile(std::array<uint32_t, 256 * 240>& buffer,
		int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors);

	std::array<uint32_t, 256 * 240> nt0;
	std::array<uint32_t, 256 * 240> nt1;
	Core* _core;
	Cartridge* _cartridge;
	PPU* _ppu;
	SharedContext* _sharedContext;
	DebuggerContext* _dbgContext;
};