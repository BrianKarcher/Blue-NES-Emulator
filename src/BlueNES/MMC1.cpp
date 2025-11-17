#include "MMC1.h"
#include "Cartridge.h"

MMC1::MMC1(Cartridge* cartridge) {
	// We use the 1 bit to track when the register is full, instead of a separate counter.
	shiftRegister = 0b10000;
	prgROMMode = 0;
	chrROMMode = 0;
	chr0Addr = 0;
	chr1Addr = 1 * CHR_SMALL;
	prg0Addr = 0;
	prg1Addr = 0;
	nametableMode = 1;
	isActive = false;
	this->cartridge = cartridge;
}

void MMC1::writeRegister(uint16_t addr, uint8_t val) {
	if ((val & 1) == 1) {
		// All five writes are in!
		shiftRegister >>= 1;
		// Record the last value that fills the register.
		// Record one bit at a time.
		shiftRegister |= ((val & 1) << 5);
		processShift(addr);
		shiftRegister = 0b10000;
	}
	// High bit resets the shift register.
	else if ((val & 0x80) == 0x80) {
		shiftRegister = 0b10000;
	}
	else {
		shiftRegister >>= 1;
		shiftRegister |= ((val & 1) << 5);
	}
}

void MMC1::processShift(uint16_t addr, uint8_t val) {
	// Control register
	if (addr >= 0x8000 && addr <= 0x9FFF) {
		isActive = true;
		uint8_t nt = val & 0b11;
		if (nt == 0) {
			cartridge->SetMirrorMode(Cartridge::MirrorMode::SINGLE_LOWER);
		}
		else if (nt == 1) {
			cartridge->SetMirrorMode(Cartridge::MirrorMode::SINGLE_UPPER);
		}
		else if (nt == 2) {
			cartridge->SetMirrorMode(Cartridge::MirrorMode::VERTICAL);
		}
		else {
			cartridge->SetMirrorMode(Cartridge::MirrorMode::HORIZONTAL);
		}
		prgROMMode = (val & 0b1100) >> 2;
		// Set the two fixed banks, if applicable.
		if (prgROMMode == 2) {
			prg0Addr = 0;
		}
		else if (prgROMMode == 3) {
			prg1Addr = PRG_SMALL;
		}
		chrROMMode = val >> 4;
	}
	// CHR Bank 0
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		chr0Addr = CHR_SMALL * val;
		if (chrROMMode == 0) {
			// 8 KB mode
			chr1Addr = chr0Addr + CHR_SMALL;
		}
	}
	// CHR Bank 1
	else if (addr >= 0xC000 && addr <= 0xDFFF) {
		chr1Addr = CHR_SMALL * val;
	}
	// PRG Bank
	else if (addr >= 0xE000 && addr <= 0xFFFF) {
		if (prgROMMode == 0 || prgROMMode == 1) {
			prg0Addr = PRG_SMALL * val;
			prg1Addr = prg0Addr + PRG_SMALL;
		}
		else if (prgROMMode == 2) {
			prg1Addr = PRG_SMALL * val;
		}
		else {
			prg0Addr = PRG_SMALL * val;
		}
	}
}