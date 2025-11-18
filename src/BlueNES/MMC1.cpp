#include "MMC1.h"
#include "Cartridge.h"
#include "INESLoader.h"
#include <Windows.h>
#include <string>
#include <bitset>

MMC1::MMC1(Cartridge* cartridge, const ines_file_t& inesFile) {
	// We use the 1 bit to track when the register is full, instead of a separate counter.
	shiftRegister = 0b10000;
	prgROMMode = 3;
	chrROMMode = 0;
	chr0Addr = 0;
	chr1Addr = 1 * CHR_SMALL;
	prg0Addr = 0;
	maxPRGBanks = inesFile.header.prg_rom_size;
	prg1Addr = (maxPRGBanks - 1) * PRG_SMALL;
	nametableMode = 1;
	this->cartridge = cartridge;
	//this->inesFile = inesFile;
}

void MMC1::writeRegister(uint16_t addr, uint8_t val) {
	wchar_t buffer[60];
	//std::wstring bits = std::to_wstring(std::bitset<8>(shiftRegister).to_string());
	swprintf_s(buffer, L"MMC 0x%08X %S %S\n", addr, std::bitset<8>(val).to_string().c_str(), std::bitset<8>(shiftRegister).to_string().c_str());  // 8-digit uppercase hex
	OutputDebugStringW(buffer);
	//OutputDebugStringW((L"MMC " + std::to_wstring(addr) + L" " + std::to_wstring(val) + L" " + std::to_wstring(shiftRegister) + L"\n").c_str());
	// High bit resets the shift register.
	if (val & 0x80) {
		shiftRegister = 0b10000;
		prgROMMode = 3;
		return;
	}

	// Shift right
	bool full = shiftRegister & 1;  // Check BEFORE shift
	shiftRegister >>= 1;
	shiftRegister |= ((val & 1) << 4);

	if (full) {
		// Now we have 5 bits
		processShift(addr, shiftRegister);
		shiftRegister = 0b10000;
	}

	//if ((shiftRegister & 1) == 1) {
	//	// All five writes are in!
	//	shiftRegister >>= 1;
	//	// Record the last value that fills the register.
	//	// Record one bit at a time.
	//	shiftRegister |= ((val & 1) << 4);
	//	wchar_t buffer2[60];
	//	swprintf_s(buffer2, L"Shift full, processing %S to 0x%08X\n", std::bitset<8>(shiftRegister).to_string().c_str(), addr);  // 8-digit uppercase hex
	//	OutputDebugStringW(buffer2);
	//	//OutputDebugStringW((L"Shift full, processing " + std::to_wstring(val) + L" to " + std::to_wstring(addr)).c_str());
	//	processShift(addr, shiftRegister);
	//	shiftRegister = 0b10000;
	//}
	//else {
	//	shiftRegister >>= 1;
	//	shiftRegister |= ((val & 1) << 4);
	//}
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
			prg1Addr = (maxPRGBanks - 1) * PRG_SMALL;
		}
		chrROMMode = val >> 4;
	}
	// CHR Bank 0
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		if (chrROMMode == 0) {
			// 8 KB mode
			val >>= 1;
		}
		chr0Addr = CHR_SMALL * val;
	}
	// CHR Bank 1
	else if (addr >= 0xC000 && addr <= 0xDFFF) {
		chr1Addr = CHR_SMALL * val;
	}
	// PRG Bank
	else if (addr >= 0xE000 && addr <= 0xFFFF) {
		// High bit does not select a PRG-ROM bank
		val &= 0b1111;
		if (prgROMMode == 0 || prgROMMode == 1) {
			// Lower bit of bank number is ignored.
			prg0Addr = PRG_SMALL * (val >> 1);
			//prg1Addr = prg0Addr + PRG_SMALL;
		}
		else if (prgROMMode == 2) {
			prg0Addr = 0;
			prg1Addr = PRG_SMALL * val;
		}
		else if (prgROMMode == 3) {
			if (val > 3) {
				int i = 0;
			}
			val %= (maxPRGBanks);
			prg0Addr = PRG_SMALL * val;
			prg1Addr = (maxPRGBanks - 1) * PRG_SMALL;
		}
	}
}

uint8_t MMC1::readPRGROM(uint16_t address) {
	if (cartridge->m_prgRomData.size() == 0) {
		return 0;
	}

	if (prgROMMode == 0 || prgROMMode == 1) {
		// 32KB mode
		int addr = prg0Addr + (address - 0x8000);
		return cartridge->m_prgRomData[addr];
	}
	else if (address >= 0x8000 && address < 0x8000 + 0x4000) {
		uint32_t addr = prg0Addr + (address - 0x8000);
		return cartridge->m_prgRomData[addr];
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
	if (chrROMMode == 0) {
		// 8 KB mode
		return cartridge->m_chrData[chr0Addr + address];
	}
	else if (address < 0x1000) {
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
	if (chrROMMode == 0) {
		// 8 KB mode
		cartridge->m_chrData[chr0Addr + address] = data;
		return;
	}
	else if (address < 0x1000) {
		cartridge->m_chrData[chr0Addr + address] = data;
	}
	else if (address < 0x2000) {
		uint16_t addr = chr1Addr + (address - 0x1000);
		cartridge->m_chrData[addr] = data;
	}
}