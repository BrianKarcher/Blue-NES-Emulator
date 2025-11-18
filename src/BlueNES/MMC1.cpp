#include "MMC1.h"
#include "Cartridge.h"
#include "INESLoader.h"
#include <Windows.h>
#include <string>
#include <bitset>

MMC1::MMC1(Cartridge* cartridge, const ines_file_t& inesFile) {
	// We use the 1 bit to track when the register is full, instead of a separate counter.
	shiftRegister = 0b10000;
	// init registers to reset-like defaults
	controlReg = 0x0C; // PRG ROM mode = 3 (fix last bank), CHR mode = 0, mirroring = 0 (single-screen lower)
	chrBank0Reg = 0;
	chrBank1Reg = 0;
	prgBankReg = 0;
	prgBankCount = inesFile.header.prg_rom_size;
	this->cartridge = cartridge;
	//this->inesFile = inesFile;
	// Initial mapping.
	recomputeMappings();
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
		controlReg |= 0x0C; // set PRG mode bits to 11 (mode 3) as hardware typically does on reset
		recomputeMappings();
		return;
	}

	// Shift right
	bool full = shiftRegister & 1;  // Check BEFORE shift
	shiftRegister >>= 1;
	shiftRegister |= ((val & 1) << 4);

	if (full) {
		// Now we have 5 bits
		processShift(addr, shiftRegister & 0x1F); // the 5-bit value
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
		// Control register (mirroring + PRG mode + CHR mode)
		controlReg = val & 0x1F;
		
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
		
	}
	// CHR Bank 0
	else if (addr >= 0xA000 && addr <= 0xBFFF) {
		// CHR bank 0 (4KB)
		chrBank0Reg = val & 0x1F;
	}
	// CHR Bank 1
	else if (addr >= 0xC000 && addr <= 0xDFFF) {
		// CHR bank 1 (4KB)
		chrBank1Reg = val & 0x1F;
	}
	// PRG Bank
	else if (addr >= 0xE000 && addr <= 0xFFFF) {
		// PRG bank register
		prgBankReg = val & 0x0F; // only low 4 bits used for PRG bank
	}
	recomputeMappings();
}

// ---------------- recomputeMappings ----------------
// Compute prg0Addr/prg1Addr/chr0Addr/chr1Addr in bytes based on current registers and sizes
void MMC1::recomputeMappings()
{
	const uint8_t prgMode = (controlReg >> 2) & 0x03; // bits 2-3
	const uint8_t chrMode = (controlReg >> 4) & 0x01; // bit 4

	// -------- CHR mapping --------
	if (chrBankCount == 0) {
		// CHR-RAM case (no CHR data present). Keep addresses at 0; reads/writes should go to CHR-RAM you provide
		chr0Addr = 0;
		chr1Addr = 0x1000;
	}
	else {
		if (chrMode == 0) {
			// 8 KB mode: ignore low bit of chrBank0Reg
			uint32_t bank4k = (chrBank0Reg & 0x1E); // even index only
			chr0Addr = bank4k * CHR_BANK_SIZE;      // maps to 0x0000-0x0FFF
			chr1Addr = chr0Addr + CHR_BANK_SIZE;    // maps to 0x1000-0x1FFF
		}
		else {
			// 4 KB mode: two independent 4KB banks
			uint32_t bank0 = chrBank0Reg;
			uint32_t bank1 = chrBank1Reg;
			// wrap to available banks
			if (chrBankCount > 0) {
				bank0 %= std::max<size_t>(1, chrBankCount);
				bank1 %= std::max<size_t>(1, chrBankCount);
			}
			chr0Addr = bank0 * CHR_BANK_SIZE;
			chr1Addr = bank1 * CHR_BANK_SIZE;
		}
	}

	// -------- PRG mapping --------
	// prgBanks16k == number of 16KB banks in PRG ROM
	if (prgBankCount == 0) {
		prg0Addr = prg1Addr = 0;
	}
	else {
		if (prgMode == 0 || prgMode == 1) {
			// 32 KB switching: ignore low bit of prgBankReg
			uint32_t bank16k = (prgBankReg & 0x0E); // force even bank (pair)
			bank16k %= std::max<size_t>(1, prgBankCount); // guard wrap
			prg0Addr = bank16k * PRG_BANK_SIZE;      // maps to 0x8000-0xBFFF
			prg1Addr = prg0Addr + PRG_BANK_SIZE;     // maps to 0xC000-0xFFFF
		}
		else if (prgMode == 2) {
			// fix first bank at 0x8000, switch 16KB bank at 0xC000
			prg0Addr = 0;
			{
				uint32_t bank = prgBankReg % std::max<size_t>(1, prgBankCount);
				prg1Addr = bank * PRG_BANK_SIZE;
			}
		}
		else { // prgMode == 3
			// fix last bank at 0xC000, switch 16KB bank at 0x8000
			uint32_t lastBank = (prgBankCount == 0 ? 0 : (prgBankCount - 1));
			prg1Addr = lastBank * PRG_BANK_SIZE;
			{
				uint32_t bank = prgBankReg % std::max<size_t>(1, prgBankCount);
				prg0Addr = bank * PRG_BANK_SIZE;
			}
		}
	}

	//dbg(L"MMC1 recompute: control=0x%02X prgMode=%d chrMode=%d\n", controlReg, (controlReg >> 2) & 3, (controlReg >> 4) & 1);
	//dbg(L"  PRG addrs: prg0=0x%06X prg1=0x%06X (prgBanks16k=%zu)\n", prg0Addr, prg1Addr, prgBanks16k);
	//dbg(L"  CHR addrs: chr0=0x%06X chr1=0x%06X (chrBanks4k=%zu)\n", chr0Addr, chr1Addr, chrBanks4k);
}

uint8_t MMC1::readPRGROM(uint16_t addr) {
	// Expect addr in 0x8000 - 0xFFFF
	if (addr < 0x8000) return 0xFF; // open bus / not mapped by mapper

	if (addr < 0xC000) {
		uint32_t offset = prg0Addr + (addr - 0x8000);
		offset %= (cartridge->m_prgRomData.size() == 0 ? 1 : cartridge->m_prgRomData.size());
		return cartridge->m_prgRomData[offset];
	}
	else {
		uint32_t offset = prg1Addr + (addr - 0xC000);
		offset %= (cartridge->m_prgRomData.size() == 0 ? 1 : cartridge->m_prgRomData.size());
		return cartridge->m_prgRomData[offset];
	}
}

void MMC1::writePRGROM(uint16_t address, uint8_t data) {
	// Typically, PRG ROM is not writable. This is a placeholder for mappers that support it.
	if (address >= 0x8000) {
		writeRegister(address, data);
		//cartridge->m_prgRomData[address - 0x8000] = data;
	}
}

uint8_t MMC1::readCHR(uint16_t addr) {
	// Expect addr 0x0000..0x1FFF
	addr &= 0x1FFF;
	if (addr < 0x1000) {
		uint32_t offset = chr0Addr + addr;
		offset %= (cartridge->m_chrData.size() == 0 ? 1 : cartridge->m_chrData.size());
		return cartridge->m_chrData.empty() ? 0 : cartridge->m_chrData[offset];
	}
	else {
		uint32_t offset = chr1Addr + (addr - 0x1000);
		offset %= (cartridge->m_chrData.size() == 0 ? 1 : cartridge->m_chrData.size());
		return cartridge->m_chrData.empty() ? 0 : cartridge->m_chrData[offset];
	}
}

// TODO: Support CHR-RAM vs CHR-ROM distinction
void MMC1::writeCHR(uint16_t address, uint8_t data) {
	if (!cartridge->isCHRWritable) {
		return; // Ignore writes if CHR is ROM
	}
	const uint8_t chrMode = (controlReg >> 4) & 0x01; // bit 4
	if (chrMode == 0) {
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