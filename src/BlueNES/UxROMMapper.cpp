#include "UxROMMapper.h"
#include "MMC3.h"
#include "CPU.h"
#include "Cartridge.h"
#include "Bus.h"
#include "nes_ppu.h"
#include "RendererLoopy.h"
#include <array>

#define BANK_SIZE_PRG 0x4000 // 16KB

// ---------------- Debug helper ----------------
inline void UxROMMapper::dbg(const wchar_t* fmt, ...) {
#ifdef MMC3DEBUG
	//if (!debug) return;
	wchar_t buf[512];
	va_list args;
	va_start(args, fmt);
	_vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
	va_end(args);
	OutputDebugStringW(buf);
#endif
}

UxROMMapper::UxROMMapper(Bus* bus, uint8_t prgRomSize, uint8_t chrRomSize) {
	this->cpu = bus->cpu;
	this->bus = bus;
	prgBank16kCount = prgRomSize;

	this->cart = bus->cart;
	uint32_t lastBankStart = (prgBank16kCount - 1) * BANK_SIZE_PRG;
	prgMap[1] = &cart->m_prgRomData[lastBankStart]; // Fixed last bank at $C000
	// Start with bank 0 selected
	prg_bank_select = 0;
	//lastPrg = &cartridge->m_prgRomData[lastBankStart];
	//secondLastPrg = &cartridge->m_prgRomData[lastBankStart - BANK_SIZE_PRG];
}

void UxROMMapper::writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) {
	dbg(L"(%d) 0x%04X 0x%02X (%d) \n", currentCycle, addr, val, val);
	// This boards uses only the lower 3 bits to select the bank
	// The bank to load is stored in ROM. The address just points to the ROM location to pull.
	//uint8_t data = cart->m_prgRomData[addr];
	//prgMap[0] = &cart->m_prgRomData[(val & (prgBank16kCount - 1)) * BANK_SIZE_PRG];
	// Any write to $8000-$FFFF sets the bank select register
	// Only lower bits are used (depends on number of banks)
	// For example, 8 banks uses 3 bits, 16 banks uses 4 bits

	uint8_t bank_mask = 0;
	if (prgBank16kCount <= 2) bank_mask = 0x01;
	else if (prgBank16kCount <= 4) bank_mask = 0x03;
	else if (prgBank16kCount <= 8) bank_mask = 0x07;
	else if (prgBank16kCount <= 16) bank_mask = 0x0F;
	else bank_mask = 0x1F;

	prg_bank_select = val & bank_mask;
}

inline uint8_t UxROMMapper::readCHR(uint16_t addr) const {
	return cart->m_chrData[addr];
}

void UxROMMapper::writeCHR(uint16_t addr, uint8_t data) {
	if (!cart->isCHRWritable) {
		return; // Ignore writes if CHR is ROM
	}
	cart->m_chrData[addr] = data;
}

inline uint8_t UxROMMapper::readPRGROM(uint16_t addr) const {
	if (addr >= 0x8000 && addr <= 0xBFFF) {
		// $8000-$BFFF: Switchable 16KB bank
		uint32_t bank_offset = prg_bank_select * 0x4000;
		uint32_t offset = bank_offset + (addr & 0x3FFF);

		// Wrap around if offset exceeds ROM size
		//offset %= prg_rom_size;

		return cart->m_prgRomData[offset];
	}
	else if (addr >= 0xC000 && addr <= 0xFFFF) {
		// $C000-$FFFF: Fixed to last 16KB bank
		uint32_t last_bank = prgBank16kCount - 1;
		uint32_t bank_offset = last_bank * 0x4000;
		uint32_t offset = bank_offset + (addr & 0x3FFF);

		return cart->m_prgRomData[offset];
	}

	return 0;  // Should not happen
	//return prgMap[(addr >> 15) & 1][addr & 0x3FFF];
	// >>14 = divide by 16384 -> index 0–3
}

void UxROMMapper::writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle) {
	if (address >= 0x8000) {
		writeRegister(address, data, currentCycle);
	}
}