#pragma once
#include <cstdint>
#include <iostream>

struct HeaderState {


};

struct CPUState {
	uint8_t m_a;
	uint8_t m_x;
	uint8_t m_y;
	int m_pc;
	uint8_t m_sp;
	uint8_t m_p;
	// Interrupt lines
	bool nmi_line;
	bool nmi_previous;
	bool nmi_pending;
	bool irq_line;
};

struct InternalMemoryState {
	uint8_t internalMemory[2048];
};

struct MapperState {

};

struct LoopyState {
	uint16_t coarse_x;  // Coarse X scroll (0-31)
	uint16_t coarse_y;  // Coarse Y scroll (0-31)
	uint16_t nametable_x;  // Nametable X bit
	uint16_t nametable_y;  // Nametable Y bit
	uint16_t fine_y;    // Fine Y scroll (0-7)
	uint16_t unused;    // Always 0
};

struct SpriteState {
	uint8_t x;
	uint8_t y;
	uint8_t tileIndex;
	uint8_t attributes;
	bool isSprite0;
};

struct RendererState {
	int m_scanline;
	LoopyState v;
	LoopyState t;
	uint8_t x;
	bool w;
	uint16_t pattern_lo_shift;
	uint16_t pattern_hi_shift;
	uint16_t attr_lo_shift;
	uint16_t attr_hi_shift;
	uint64_t _frameCount; // We may not need this, it's to do odd/even frame handling
	uint8_t ppumask = 0;
	int dot = 0;
	SpriteState secondaryOAM[8];
};

struct PPUState {
	RendererState renderer;
	uint8_t oam[0x100];
	uint8_t oamAddr;
	uint8_t paletteTable[32];
	uint8_t ppuMask;
	uint8_t ppuStatus;
	uint8_t ppuCtrl;
	uint8_t vram[0x800];
	uint8_t ppuDataBuffer;
};

struct SaveState {
	HeaderState header;
	CPUState cpu;
	PPUState ppu;
	InternalMemoryState memory;
	MapperState mapper;
	// OAM DMA
	bool dmaActive;
	uint8_t dmaPage;
	uint8_t dmaAddr;
	uint16_t dmaCycles;
};