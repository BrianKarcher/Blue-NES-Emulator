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
	// Bank counts are in 4KB's, chr_rom_size is in 8KB units.
	chrBankCount = inesFile.header.chr_rom_size * 2;
	// Detect board type from PRG/CHR configuration
	if (prgBankCount == 4 && chrBankCount == 4) {
		boardType = BoardType::SAROM;   // 64KB PRG, 16KB CHR-ROM
	}
	else if (prgBankCount == 8 && chrBankCount == 0) {
		boardType = BoardType::SNROM;   // 128KB PRG, CHR-RAM
	}
	else {
		boardType = BoardType::GenericMMC1;
	}
	this->cartridge = cartridge;
	//this->inesFile = inesFile;
	// Initial mapping.
	recomputeMappings();
}

// ---------------- Debug helper ----------------
void MMC1::dbg(const wchar_t* fmt, ...) const {
	if (!debug) return;
	wchar_t buf[512];
	va_list args;
	va_start(args, fmt);
	_vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
	va_end(args);
	OutputDebugStringW(buf);
}

void MMC1::writeRegister(uint16_t addr, uint8_t val) {
	// Debug print
	std::string bits = std::bitset<8>(shiftRegister).to_string();
	std::wstring wbits(bits.begin(), bits.end());
	dbg(L"MMC1 write 0x%04X = 0x%02X  shiftReg=0b%s\n",
		addr, val, wbits.c_str());
	wchar_t buffer[60];
	//std::wstring bits = std::to_wstring(std::bitset<8>(shiftRegister).to_string());
	//swprintf_s(buffer, L"MMC 0x%08X %S %S\n", addr, std::bitset<8>(val).to_string().c_str(), std::bitset<8>(shiftRegister).to_string().c_str());  // 8-digit uppercase hex
	//OutputDebugStringW(buffer);
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
	std::string bits = std::bitset<8>(val).to_string();
	std::wstring wbits(bits.begin(), bits.end());
	dbg(L"MMC1 shift full for 0x%04X -> data=0b%s (0x%02X)\n",
		addr, wbits.c_str(), val);
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
	// --- Clamp CHR bank values for SAROM ---
	if (boardType == BoardType::SAROM) {
		// SAROM only has 16KB of CHR-ROM
		chrBank0Reg &= 0x04;
		chrBank1Reg &= 0x04;
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
	// Remove the early return - always compute banking even for CHR-RAM!
	if (chrBankCount == 0) {
		// CHR-RAM: always 8KB, no real banking
		chr0Addr = 0;
		chr1Addr = 0x1000;
	}
	else if (chrMode == 0) {
		// 8KB mode
		uint32_t bank8k = chrBank0Reg;
		if (boardType == BoardType::SAROM)
			bank8k &= 0x04;                     // SAROM: only 4 banks (16KB total)
		bank8k %= chrBankCount;
		chr0Addr = bank8k * 0x2000;
		chr1Addr = chr0Addr + 0x1000;
	}
	else {
		// 4KB mode
		uint32_t bank0 = chrBank0Reg % chrBankCount;
		uint32_t bank1 = chrBank1Reg % chrBankCount;
		chr0Addr = bank0 * 0x1000;
		chr1Addr = bank1 * 0x1000;
	}

	// ------------ PRG BANKING ------------
	uint32_t prgMax = prgBankCount - 1;
	uint32_t bank = prgBankReg & prgMax;
	uint32_t lastBankStart = (prgBankCount - 1) * 0x4000;

	switch (prgMode) {

	case 0:
	case 1:
		// 32 KB mode
	{
		prg0Addr = ((prgBankReg & 0xFE) % prgBankCount) * 0x4000;  // even bank
		prg1Addr = prg0Addr + 0x4000;
	}
	break;

	case 2:
		// First 16KB fixed at $8000
		prg0Addr = 0;
		prg1Addr = (prgBankReg % prgBankCount) * 0x4000;
		break;

	case 3:
	default:
		// Last 16KB fixed at $C000
		prg0Addr = (prgBankReg % prgBankCount) * 0x4000;
		prg1Addr = lastBankStart;
		break;
	}

	dbg(L"MMC1 recompute: control=0x%02X prgMode=%d chrMode=%d\n", controlReg, (controlReg >> 2) & 3, (controlReg >> 4) & 1);
	dbg(L"  PRG addrs: prg0=0x%06X prg1=0x%06X (prgBankCount=%d)\n", prg0Addr, prg1Addr, prgBankCount);
	dbg(L"  CHR addrs: chr0=0x%06X chr1=0x%06X (chrBankCount=%d)\n", chr0Addr, chr1Addr, chrBankCount);
}

uint8_t MMC1::readPRGROM(uint16_t addr) {
	// Expect addr in 0x8000 - 0xFFFF
	if (addr < 0x8000) return 0xFF; // open bus / not mapped by mapper

	uint32_t offset;
	if (addr < 0xC000)
		offset = prg0Addr + (addr & 0x3FFF);
	else
		offset = prg1Addr + (addr & 0x3FFF);

	offset %= cartridge->m_prgRomData.size();
	return cartridge->m_prgRomData[offset];
}

void MMC1::writePRGROM(uint16_t address, uint8_t data) {
	// Typically, PRG ROM is not writable. This is a placeholder for mappers that support it.
	if (address >= 0x8000) {
		writeRegister(address, data);
		//cartridge->m_prgRomData[address - 0x8000] = data;
	}
}

uint8_t MMC1::readCHR(uint16_t addr) {
	addr &= 0x1FFF;
	uint32_t offset = (addr < 0x1000) ? (chr0Addr + addr) : (chr1Addr + (addr - 0x1000));

	if (cartridge->m_chrData.empty())
		return 0;

	offset %= cartridge->m_chrData.size();
	return cartridge->m_chrData[offset];
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