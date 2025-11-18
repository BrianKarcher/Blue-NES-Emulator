#include "MMC1.h"
#include "Cartridge.h"
#include "INESLoader.h"
#include <Windows.h>
#include <string>

MMC1::MMC1(Cartridge* cartridge, const ines_file_t& inesFile) {
	// We use the 1 bit to track when the register is full, instead of a separate counter.
	shiftRegister = 0b10000;
	prgROMMode = 0;
	chrROMMode = 0;
	chr0Addr = 0;
	chr1Addr = 1 * CHR_SMALL;
	prg0Addr = 0;
	prg1Addr = (inesFile.header.prg_rom_size - 1) * PRG_SMALL;
	nametableMode = 1;
	this->cartridge = cartridge;
	//this->inesFile = inesFile;
}

void MMC1::writeRegister(uint16_t addr, uint8_t val) {
	if ((shiftRegister & 1) == 1) {
		// All five writes are in!
		shiftRegister >>= 1;
		// Record the last value that fills the register.
		// Record one bit at a time.
		shiftRegister |= ((val & 1) << 5);
		processShift(addr, shiftRegister);
		shiftRegister = 0b10000;
	}
	// High bit resets the shift register.
	else if ((val & 0x80) == 0x80) {
		shiftRegister = 0b10000;
	}
	else {
		shiftRegister >>= 1;
		shiftRegister |= ((val & 1) << 4);
	}
	OutputDebugStringW((L"MMC " + std::to_wstring(addr) + L" " + std::to_wstring(val) + L" " + std::to_wstring(shiftRegister) + L"\n").c_str());
}

void MMC1::processShift(uint16_t addr, uint8_t val) {
	// Control register
	if (addr >= 0x8000 && addr <= 0x9FFF) {
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
			// Lock the first bank
			prg0Addr = 0;
		}
		else if (prgROMMode == 3) {
			// Lock the last bank
			//prg1Addr = (inesFile->header->prg_rom_size - 1) * PRG_SMALL;
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

uint8_t MMC1::readPRGROM(uint16_t address) {
	if (cartridge->m_prgRomData.size() == 0) {
		return 0;
	}

	if (address >= 0x8000 && address < 0x8000 + 0x4000) {
		return cartridge->m_prgRomData[prg0Addr + (address - 0x8000)];
	}
	else if (address >= 0xC000) {
		uint32_t addr = prg1Addr + (address - 0xC000);
		return cartridge->m_prgRomData[addr];
	}
	return 0;
}

void MMC1::writePRGROM(uint16_t address, uint8_t data) {
	// Typically, PRG ROM is not writable. This is a placeholder for mappers that support it.
	if (address >= 0x8000) {
		writeRegister(address, data);
		//cartridge->m_prgRomData[address - 0x8000] = data;
	}
}

uint8_t MMC1::readCHR(uint16_t address) {
	if (address < 0x1000) {
		return cartridge->m_chrData[chr0Addr + address];
	}
	else if (address < 0x2000) {
		return cartridge->m_chrData[chr1Addr + (address - 0x1000)];
	}
	return 0;
}

// TODO: Support CHR-RAM vs CHR-ROM distinction
void MMC1::writeCHR(uint16_t address, uint8_t data) {
	if (!cartridge->isCHRWritable) {
		return; // Ignore writes if CHR is ROM
	}
	if (address < 0x1000) {
		cartridge->m_chrData[chr0Addr + address] = data;
	}
	else if (address < 0x2000) {
		cartridge->m_chrData[chr1Addr + (address - 0x1000)] = data;
	}
}