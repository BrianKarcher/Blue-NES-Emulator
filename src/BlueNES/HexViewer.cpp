#include "HexViewer.h"
#include "imgui.h"
#include <cstdint>
#include "SharedContext.h"
#include "Core.h"
#include "DebuggerContext.h"

HexViewer::HexViewer(Core* core, SharedContext& sharedCtx) : _sharedCtx(sharedCtx), _dbgCtx(sharedCtx.debugger_context) {
    _bus = core->emulator.GetBus();
    hexSources[0] = HexReadCPU;
	hexSources[1] = HexReadPPU;
}

uint8_t HexReadPPU(HexViewer hexViewer, uint16_t addr) {
    if (addr < 0x2000) {
        return hexViewer._dbgCtx->ppuState.chrMemory[addr];
    }
    else if (addr < 0x3F00) {
        // Reading from nametables and attribute tables
        uint16_t mirroredAddr = addr & 0x2FFF; // Mirror nametables every 4KB
        mirroredAddr = hexViewer._bus->cart.MirrorNametable(mirroredAddr);
        return hexViewer._dbgCtx->ppuState.nametables[mirroredAddr];
    }
    else if (addr < 0x4000) {
        // Reading from palette RAM (mirrored every 32 bytes)
        uint8_t paletteAddr = addr & 0x1F;
        if (paletteAddr >= 0x10 && (paletteAddr % 4 == 0)) {
            paletteAddr -= 0x10; // Mirror universal background color
        }
        return hexViewer._dbgCtx->ppuState.palette[paletteAddr];
    }
    //return core->emulator.nes.ppu.ReadVRAM(val);
    return 0;
}

uint8_t HexReadCPU(HexViewer hexViewer, uint16_t addr) {
	return hexViewer._bus->peek(addr);
}

void HexViewer::DrawMemoryViewer(const char* title, size_t size) {
    ImGui::Begin(title);

    // Set a default size for the viewer
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);

    // We use 1 column for the Address, 16 for Hex data, and 1 for ASCII
    static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;

    if (ImGui::BeginTable("MemTable", 18, flags, ImVec2(0, ImGui::GetContentRegionAvail().y))) {
        // Setup columns
        ImGui::TableSetupColumn("Addr", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        for (int i = 0; i < 16; i++) ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 25.0f);
        ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        // Use Clipper: each row displays 16 bytes
        ImGuiListClipper clipper;
        clipper.Begin((int)((size + 15) / 16));

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                ImGui::TableNextRow();

                // Column 0: Address
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%04X:", row * 16);

                // Columns 1-16: Hex Bytes
                for (int n = 0; n < 16; n++) {
                    ImGui::TableSetColumnIndex(n + 1);
                    size_t addr = row * 16 + n;
                    if (addr < size) {
                        uint8_t val = _bus->peek(addr);

                        // Highlight non-zero values or specific addresses (like Stack or Zero Page)
                        if (val == 0) ImGui::TextDisabled("00");
                        else ImGui::Text("%02X", val);
                    }
                }

                // Column 17: ASCII representation
                ImGui::TableSetColumnIndex(17);
                char ascii[17];
                for (int n = 0; n < 16; n++) {
                    size_t addr = row * 16 + n;
                    uint8_t c = (addr < size) ? _bus->peek(addr) : ' ';
                    ascii[n] = (c >= 32 && c <= 126) ? c : '.';
                }
                ascii[16] = '\0';
                ImGui::TextUnformatted(ascii);
            }
        }
        ImGui::EndTable();
    }
    ImGui::End();
}