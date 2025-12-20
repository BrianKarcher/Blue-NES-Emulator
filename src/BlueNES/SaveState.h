#pragma once
#include <cstdint>
#include <iostream>

typedef struct {


} HeaderState;

typedef struct {
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
} CPUState;

typedef struct {
	uint8_t internalMemory[2048];
} InternalMemoryState;

typedef struct {

} MapperState;

typedef struct {
	uint16_t coarse_x : 5;  // Coarse X scroll (0-31)
	uint16_t coarse_y : 5;  // Coarse Y scroll (0-31)
	uint16_t nametable_x : 1;  // Nametable X bit
	uint16_t nametable_y : 1;  // Nametable Y bit
	uint16_t fine_y : 3;    // Fine Y scroll (0-7)
	uint16_t unused : 1;    // Always 0
} LoopyState;

typedef struct Sprite {
	uint8_t x;
	uint8_t y;
	uint8_t tileIndex;
	uint8_t attributes;
	bool isSprite0;
} SpriteState;

typedef struct {
	int m_scanline = 0;
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
} RendererState;

typedef struct {
	HeaderState header;
	CPUState cpu;
	InternalMemoryState memory;
	MapperState mapper;
	// OAM DMA
	bool dmaActive;
	uint8_t dmaPage;
	uint8_t dmaAddr;
	uint16_t dmaCycles;
} SaveState;