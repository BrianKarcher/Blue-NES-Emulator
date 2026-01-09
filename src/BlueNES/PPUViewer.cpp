#include "PPUViewer.h"
#include "Bus.h"
#include "Core.h"
#include "EmulatorCore.h"
#include "Nes.h"
#include "imgui.h"

bool PPUViewer::Initialize(Core* core, SharedContext* sharedCtx) {
    _core = core;
	_bus = core->emulator.GetBus();
    _ppu = core->emulator.GetPPU();
    _cartridge = core->emulator.GetCartridge();
    _sharedContext = sharedCtx;
    for (int i = 0; i < 2; i++) {
        glGenTextures(1, &ntTextures[i]);
        glBindTexture(GL_TEXTURE_2D, ntTextures[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 240, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
        //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 240, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    return true;
}

void PPUViewer::UpdateTexture(int index, std::array<uint32_t, 256 * 240>& data) {
    glBindTexture(GL_TEXTURE_2D, ntTextures[index]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data.data());
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind to avoid state bleeding
}

// TODO Improve speed with dirty rectangles
// Also, cache the two nametables and only change them when needed (bank switch or write).
void PPUViewer::Draw(const char* title, bool* open)
{
    if (!ImGui::Begin(title, open)) { ImGui::End(); return; }
    if (_core->isPlaying && _sharedContext->coreRunning.load(std::memory_order_relaxed)) {
        renderNametable(nt0, 0);
		UpdateTexture(0, nt0);
        renderNametable(nt1, 1);
		UpdateTexture(1, nt1);

        // Logic to arrange the 4 slots based on mirroring
            // 0: Top-Left, 1: Top-Right, 2: Bottom-Left, 3: Bottom-Right
        int slots[4];
        if (_cartridge->GetMirrorMode() == Cartridge::MirrorMode::HORIZONTAL) {
            slots[0] = 0; slots[1] = 0; slots[2] = 1; slots[3] = 1;
        }
        else {
            slots[0] = 0; slots[1] = 1; slots[2] = 0; slots[3] = 1;
        }

        ImGui::BeginGroup();
        float w = 256, h = 240; // You can scale these

        // Top Row
        ImGui::Image((void*)(intptr_t)ntTextures[slots[0]], ImVec2(w, h)); ImGui::SameLine();
        ImGui::Image((void*)(intptr_t)ntTextures[slots[1]], ImVec2(w, h));

        // Bottom Row
        ImGui::Image((void*)(intptr_t)ntTextures[slots[2]], ImVec2(w, h)); ImGui::SameLine();
        ImGui::Image((void*)(intptr_t)ntTextures[slots[3]], ImVec2(w, h));

        ImGui::EndGroup();
    }
    ImGui::End();
}

void PPUViewer::renderNametable(std::array<uint32_t, 256 * 240>& buffer, int physicalTable)
{
    // Clear back buffer
    // m_backBuffer.fill(0xFF000000);

    // Nametable starts at 0x2000 in VRAM
    // TODO : Support multiple nametables and mirroring
    //const uint16_t nametableAddr = 0x2000;
    const uint16_t nametableAddr = physicalTable * 0x400;

    // TODO: For now, we just render the nametable directly without scrolling or attribute tables
    std::array<uint32_t, 4> palette;
    // Render the 32x30 tile nametable
    for (int row = 0; row < 30; row++) {
        for (int col = 0; col < 32; col++) {
            // Get the tile index from the nametable in VRAM
            uint8_t tileIndex = _ppu->m_vram[nametableAddr + row * 32 + col];
            if (tileIndex != 0xff) {
				int i = 0;
            }

            // Calculate pixel coordinates
            int pixelX = col * 8;  // Each tile is 8x8 pixels
            int pixelY = row * 8;

            int attrRow = row / 4;
            int attrCol = col / 4;
            // Get attribute byte for the tile
            uint16_t attrAddr = (nametableAddr | 0x3c0) + attrRow * 8 + attrCol;
            uint8_t attributeByte = _ppu->m_vram[attrAddr];
            if (attributeByte > 0) {
                int i = 0;
            }

            uint8_t paletteIndex = 0;
            get_palette_index_from_attribute(attributeByte, row, col, paletteIndex);
            _ppu->get_palette(paletteIndex, palette);
            // Render the tile at the calculated position
            render_tile(buffer, pixelY, pixelX, tileIndex, palette);
        }
    }
}

void PPUViewer::render_tile(std::array<uint32_t, 256 * 240>& buffer,
    int pr, int pc, int tileIndex, std::array<uint32_t, 4>& colors) {
    
    int tileOffset = tileIndex * 16; // 16 bytes per tile
    // Determine the pattern table base address
    uint16_t patternTableBase = _ppu->GetBackgroundPatternTableBase();

    for (int y = 0; y < 8; y++) {
        
        int tileBase = patternTableBase + tileOffset; // 16 bytes per tile

        uint8_t byte1 = _bus->cart.mapper->readCHR(tileBase + y);     // bitplane 0
        uint8_t byte2 = _bus->cart.mapper->readCHR(tileBase + y + 8); // bitplane 1

        for (int x = 0; x < 8; x++) {
            uint8_t bit0 = (byte1 >> (7 - x)) & 1;
            uint8_t bit1 = (byte2 >> (7 - x)) & 1;
            uint8_t colorIndex = (bit1 << 1) | bit0;

            uint32_t actualColor = 0;
            if (colorIndex == 0) {
                actualColor = m_nesPalette[_ppu->paletteTable[0]] | 0xFF000000; // Transparent color (background color)
            }
            else {
                actualColor = colors[colorIndex] | 0xFF000000; // Map to actual color from palette
            }
            int renderX = pc + x;
            int renderY = pr + y;
            if (renderX < 0 || renderY < 0 || renderX >= 256 || renderY >= 240)
                continue;
            buffer[(renderY * 256) + renderX] = actualColor;
        }
    }
}

void PPUViewer::get_palette_index_from_attribute(uint8_t attributeByte, int tileRow, int tileCol, uint8_t& paletteIndex)
{
    // Each attribute byte covers a 4x4 tile area (32x32 pixels)
    // Determine which quadrant of the attribute byte the tile is in
    int quadrantRow = (tileRow % 4) / 2; // 0 or 1
    int quadrantCol = (tileCol % 4) / 2; // 0 or 1
    // Extract the corresponding 2 bits for the palette index
    switch (quadrantRow) {
    case 0:
        switch (quadrantCol) {
        case 0:
            paletteIndex = attributeByte & 0x03; // Bits 0-1
            break;
        case 1:
            paletteIndex = (attributeByte >> 2) & 0x03; // Bits 2-3
            break;
        }
        break;
    case 1:
        switch (quadrantCol) {
        case 0:
            paletteIndex = (attributeByte >> 4) & 0x03; // Bits 4-5
            break;
        case 1:
            paletteIndex = (attributeByte >> 6) & 0x03; // Bits 6-7
            break;
        }
        break;
    }
}