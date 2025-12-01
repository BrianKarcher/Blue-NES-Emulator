#include "nes_ppu.h"
#include <string>
#include <cstdint>
#include <WinUser.h>
#include "Bus.h"
#include "Core.h"
#include "RendererWithReg.h"
#include "RendererSlow.h"
#include "RendererLoopy.h"
#include "A12Mapper.h"

HWND m_hwnd;

NesPPU::NesPPU() {
	oam.fill(0xFF);
	m_ppuCtrl = 0;
	oamAddr = 0;
}

NesPPU::~NesPPU()
{
	if (renderer)
		delete renderer;
}

void NesPPU::Initialize(Bus* bus, Core* core) {
	this->bus = bus;
	this->core = core;
	renderer = new RendererLoopy();
	renderer->initialize(this);
}

void NesPPU::set_hwnd(HWND hwnd) {
	m_hwnd = hwnd;
}

void NesPPU::reset()
{
	// Reset PPU state here
	renderer->reset();
}

void NesPPU::step()
{
	// Emulate one PPU cycle here

}

void NesPPU::write_register(uint16_t addr, uint8_t value)
{
	// addr is in the range 0x2000 to 0x2007
	// It is the CPU that writes to these registers
	// addr is mirrored every 8 bytes up to 0x3FFF so we mask it
	switch (addr) {
	case PPUCTRL:
		dbg(L"(%d) 0x%04X PPUCTRL Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		m_ppuCtrl = value;
		//OutputDebugStringW((L"PPUCTRL: " + std::to_wstring(value) + L"\n").c_str());
		renderer->setPPUCTRL(value);
		break;
	case PPUMASK: // PPUMASK
		dbg(L"(%d) 0x%04X PPUMASK Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		m_ppuMask = value;
		renderer->setPPUMask(value);
		break;
	case PPUSTATUS: // PPUSTATUS (read-only)
		dbg(L"(%d) 0x%04X PPUSTATUS Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		// Ignore writes to PPUSTATUS
		break;
	case OAMADDR: // OAMADDR
		dbg(L"(%d) 0x%04X OAMADDR Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		oamAddr = value;
		break;
	case OAMDATA:
		dbg(L"(%d) 0x%04X OAMDATA Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		oam[oamAddr++] = value;
		break;
	case PPUSCROLL:
		dbg(L"(%d) 0x%04X PPUSCROLL Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		renderer->writeScroll(value);
		break;
	case PPUADDR: // PPUADDR
		dbg(L"(%d) 0x%04X PPUADDR Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		if (value == 0x3F) {
			int i = 0;
		}
		renderer->ppuWriteAddr(value);
		break;
	case PPUDATA: // PPUDATA
		dbg(L"(%d) 0x%04X PPUDATA Write 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		uint16_t vramAddr = renderer->getPPUAddr();
		if (vramAddr >= 0x3F00) {
			int i = 0;
		}
		renderer->ppuIncrementVramAddr(m_ppuCtrl & PPUCTRL_INCREMENT ? 32 : 1);
		write_vram(vramAddr, value);
		break;
	}
}

void NesPPU::SetVRAMAddress(uint16_t addr) {
	renderer->ppuWriteAddr(addr >> 8);
	renderer->ppuWriteAddr(addr);
	//vramAddr = addr & 0x3FFF;
}

uint8_t NesPPU::read_register(uint16_t addr)
{
	switch (addr)
	{
	case PPUCTRL:
	{
		dbg(L"(%d) 0x%04X PPUCTRL Read 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), m_ppuCtrl);
		return m_ppuCtrl;
	}
	case PPUMASK:
	{
		dbg(L"(%d) 0x%04X PPUMASK Read\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC());
		// not typically readable, return 0
		return 0;
	}
	case PPUSTATUS:
	{
		renderer->ppuReadStatus(); // Reset write toggle on reading PPUSTATUS
		// Return PPU status register value and clear VBlank flag
		uint8_t status = m_ppuStatus;
		dbg(L"(%d) 0x%04X PPUSTATUS Read 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), status);
		m_ppuStatus &= ~PPUSTATUS_VBLANK;
		return status;
	}
	case OAMADDR:
	{
		dbg(L"(%d) 0x%04X OAMADDR Read 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), oamAddr);
		return oamAddr;
	}
	case OAMDATA:
	{
		dbg(L"(%d) 0x%04X OAMDATA Read 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), oam[oamAddr]);
		// Return OAM data at current OAMADDR
		return oam[oamAddr];
	}
	case PPUSCROLL:
	{
		dbg(L"(%d) 0x%04X PPUSCROLL Read\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC());
		// PPUSCROLL is write-only, return 0
		return 0;
	}
	case PPUADDR:
	{
		dbg(L"(%d) 0x%04X PPUADDR Read\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC());
		// PPUADDR is write-only, return 0
		return 0;
	}
	case PPUDATA:
	{
		// Read from VRAM at current vramAddr
		uint16_t vramAddr = renderer->getPPUAddr();
		uint8_t value = 0;
		if (vramAddr < 0x3F00) {
			// PPU reading is through a buffer and the results are off by one.
			value = ppuDataBuffer;
			ppuDataBuffer = ReadVRAM(vramAddr);
			renderer->ppuIncrementVramAddr(m_ppuCtrl & PPUCTRL_INCREMENT ? 32 : 1); // increment v
		}
		else {
			// I probably shouldn't support reading palette data
			// as some NES's themselves don't support it. No game should be doing this.
			// Reading palette data is grabbed right away
			// without the buffer, and the buffer is filled with the underlying nametable byte
			
			// Reading from palette RAM (mirrored every 32 bytes)
			uint8_t paletteAddr = vramAddr & 0x1F;
			if (paletteAddr >= 0x10 && (paletteAddr % 4 == 0)) {
				paletteAddr -= 0x10; // Mirror universal background color
			}
			value = paletteTable[paletteAddr];
			// But buffer is filled with the underlying nametable byte
			uint16_t mirroredAddr = vramAddr & 0x2FFF; // Mirror nametables every 4KB
			mirroredAddr = bus->cart->MirrorNametable(mirroredAddr);
			ppuDataBuffer = m_vram[mirroredAddr];
			renderer->ppuIncrementVramAddr(m_ppuCtrl & PPUCTRL_INCREMENT ? 32 : 1); // increment v
		}
		//// Increment VRAM address based on PPUCTRL setting
		//if (vramAddr >= 0x3F00) {
		//	// Palette data always increments by 1
		//	vramAddr += 1;
		//	return value;
		//}
		//vramAddr += m_ppuCtrl & 0x04 ? 32 : 1;
		dbg(L"(%d) 0x%04X PPUDATA Read 0x%02X\n", bus->cpu->GetCycleCount(), bus->cpu->GetPC(), value);
		return value;
	}
	}

	return 0;
}

uint8_t NesPPU::GetScrollX() const {
	return renderer->getScrollX();
}

uint8_t NesPPU::GetScrollY() const {
	return renderer->getScrollY();
}

uint16_t NesPPU::GetVRAMAddress() const {
	return renderer->getPPUAddr();
}

uint8_t NesPPU::ReadVRAM(uint16_t addr)
{
	uint8_t value = 0;
	if (addr < 0x2000) {
		// Reading from CHR-ROM/RAM
		value = bus->cart->ReadCHR(addr);
		//if (m_mapper) {
		//	if (addr >= 0x1000) {
		//		int i = 0;
		//	}
		//	m_mapper->ClockIRQCounter(addr);
		//}
	}
	else if (addr < 0x3F00) {
		// Reading from nametables and attribute tables
		uint16_t mirroredAddr = addr & 0x2FFF; // Mirror nametables every 4KB
		mirroredAddr = bus->cart->MirrorNametable(mirroredAddr);
		value = m_vram[mirroredAddr];
	}
	else if (addr < 0x4000) {
		// Reading from palette RAM (mirrored every 32 bytes)
		uint8_t paletteAddr = addr & 0x1F;
		if (paletteAddr >= 0x10 && (paletteAddr % 4 == 0)) {
			paletteAddr -= 0x10; // Mirror universal background color
		}
		value = paletteTable[paletteAddr];
	}

	return value;
}

void NesPPU::write_vram(uint16_t addr, uint8_t value)
{
	addr &= 0x3FFF; // Mask to 14 bits
	if (addr < 0x2000) {
		// Write to CHR-RAM (if enabled)
		bus->cart->WriteCHR(addr, value);
		//if (m_mapper) {
		//	m_mapper->ClockIRQCounter(addr);
		//}
		// Else ignore write (CHR-ROM is typically read-only)
		return;
	}
	else if (addr < 0x3F00) {
		// Name tables and attribute tables
		addr &= 0x2FFF; // Mirror nametables every 4KB
		addr = bus->cart->MirrorNametable(addr);
		m_vram[addr] = value;
		return;
	}
	else if (addr < 0x4000) {
		// Palette RAM (mirrored every 32 bytes)
		// 3F00 = 0011 1111 0000 0000
		// 3F1F = 0011 1111 0001 1111
		uint8_t paletteAddr = addr & 0x1F; // 0001 1111
		if (paletteAddr % 4 == 0) {
			// Handle special mirroring of background color entries
			// These 4 addresses mirror to their lower counterparts
			if (paletteAddr == 0x10) paletteAddr = 0x00;
			if (paletteAddr == 0x14) paletteAddr = 0x04;
			if (paletteAddr == 0x18) paletteAddr = 0x08;
			if (paletteAddr == 0x1C) paletteAddr = 0x0C;
		}
		paletteTable[paletteAddr] = value;
		InvalidateRect(core->m_hwndPalette, NULL, FALSE); // Update palette window if open
		return;
	}
}

const std::array<uint32_t, 256 * 240>& NesPPU::get_back_buffer() {
	return renderer->getBackBuffer();
}

// OAM DMA - Direct Memory Access for sprites
//void NesPPU::OAMDMA(uint8_t* cpuMemory, uint16_t page) {
//	uint16_t addr = page << 8;
//	for (int i = 0; i < 256; i++) {
//		oam[oamAddr++] = cpuMemory[addr + i];
//	}
//	// OAM DMA takes 513 or 514 CPU cycles depending on odd/even alignment
//	// OAMADDR wraps around automatically in hardware
//	oamAddr &= 0xFF;
//}

void NesPPU::Clock() {
	// TODO - Send to renderer
	renderer->clock();
}

bool NesPPU::NMI() {
	bool hasVBlank = m_ppuStatus & PPUSTATUS_VBLANK;
	if (hasVBlank) {
		// Disable blanking after NMI is triggered
		m_ppuStatus &= ~PPUSTATUS_VBLANK;
	}
	return hasVBlank && (m_ppuCtrl & 0x80);
}

uint8_t NesPPU::get_tile_pixel_color_index(uint8_t tileIndex, uint8_t pixelInTileX, uint8_t pixelInTileY, bool isSprite, bool isSecondSprite)
{
	if (isSprite) {
		int i = 0;
	}
	// Determine the pattern table base address
	uint16_t patternTableBase = isSprite ? GetSpritePatternTableBase(tileIndex) : GetBackgroundPatternTableBase();
	// This needs to be refactored, it's hard to understand.
	if (isSprite) {
		if (m_ppuCtrl & PPUCTRL_SPRITESIZE) { // 8x16
			if (tileIndex & 1) {
				if (!isSecondSprite) {
					tileIndex -= 1;
				}
			}
			else {
				if (isSecondSprite) {
					tileIndex += 1;
				}
			}
		}
	}
	
	int tileBase = patternTableBase + (tileIndex * 16); // 16 bytes per tile

	uint8_t byte1 = bus->cart->ReadCHR(tileBase + pixelInTileY);     // bitplane 0
	uint8_t byte2 = bus->cart->ReadCHR(tileBase + pixelInTileY + 8); // bitplane 1

	uint8_t bit0 = (byte1 >> (7 - pixelInTileX)) & 1;
	uint8_t bit1 = (byte2 >> (7 - pixelInTileX)) & 1;
	uint8_t colorIndex = (bit1 << 1) | bit0;

	if (colorIndex != 0)
	{
		int i = 0;
	}

	return colorIndex;
}

void NesPPU::SetPPUStatus(uint8_t flag) {
	m_ppuStatus |= flag;
}

void NesPPU::get_palette(uint8_t paletteIndex, std::array<uint32_t, 4>& colors)
{
	// Each palette consists of 4 colors, starting from 0x3F00 in VRAM
	uint16_t paletteAddr = paletteIndex * 4;
	colors[0] = m_nesPalette[paletteTable[paletteAddr] & 0x3F];
	colors[1] = m_nesPalette[paletteTable[paletteAddr + 1] & 0x3F];
	colors[2] = m_nesPalette[paletteTable[paletteAddr + 2] & 0x3F];
	colors[3] = m_nesPalette[paletteTable[paletteAddr + 3] & 0x3F];
}

bool NesPPU::isFrameComplete() {
	return renderer->isFrameComplete();
}

void NesPPU::setFrameComplete(bool complete) {
	renderer->setFrameComplete(complete);
}

// ---------------- Debug helper ----------------
inline void NesPPU::dbg(const wchar_t* fmt, ...) {
#ifdef PPUDEBUG
	//if (!debug) return;
	wchar_t buf[512];
	va_list args;
	va_start(args, fmt);
	_vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
	va_end(args);
	OutputDebugStringW(buf);
#endif
}