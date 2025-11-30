#include "MMC3.h"
#include "CPU.h"
#include "Cartridge.h"

#define BANK_SIZE_CHR 0x400 // 1KB
#define BANK_SIZE_PRG 0x2000 // 8KB

MMC3::MMC3(Cartridge* cartridge, Processor_6502* cpu, uint8_t prgRomSize, uint8_t chrRomSize) {
	this->cpu = cpu;
	// init registers to reset-like defaults
	prgMode = 0;
	chrMode = 0;
	prgBank16kCount = prgRomSize;
	prgBank8kCount = prgBank16kCount * 2;
	chrBank8kCount = chrRomSize;
	chrBank1kCount = chrBank8kCount * 8;
	for (int i = 0; i < 8; i++) {
		banks[i] = 0;
	}

	this->cart = cartridge;
	uint32_t lastBankStart = (prgBank8kCount - 1) * BANK_SIZE_PRG;
	//lastPrg = &cartridge->m_prgRomData[lastBankStart];
	//secondLastPrg = &cartridge->m_prgRomData[lastBankStart - BANK_SIZE_PRG];
	updatePrgMapping();
	updateChrMapping();
}

void MMC3::updateChrMapping() {
	if (chrMode) {
		// $0000-$03FF: R2 (fine)
		// $0400-$07FF: R3
		// $0800-$0BFF: R4
		// $0C00-$0FFF: R5
		// $1000-$13FF: R0 (2KB split)
		// $1400-$17FF: R0 + 1KB
		// $1800-$1BFF: R1 (2KB split)
		// $1C00-$1FFF: R1 + 1KB

		chrMap[0] = &cart->m_chrData[banks[2] * 0x400];
		chrMap[1] = &cart->m_chrData[banks[3] * 0x400];
		chrMap[2] = &cart->m_chrData[banks[4] * 0x400];
		chrMap[3] = &cart->m_chrData[banks[5] * 0x400];
		chrMap[4] = &cart->m_chrData[banks[0] * 0x400];           // R0 even
		chrMap[5] = &cart->m_chrData[banks[0] * 0x400 + 0x400];
		chrMap[6] = &cart->m_chrData[banks[1] * 0x400];           // R1 even
		chrMap[7] = &cart->m_chrData[banks[1] * 0x400 + 0x400];
	}
	else {
		// R0 covers $0000-$0FFF (two 1KB slots)
		// R1 covers $1000-$1FFF
		chrMap[0] = &cart->m_chrData[banks[0] * 0x400];
		chrMap[1] = &cart->m_chrData[banks[0] * 0x400 + 0x400];
		chrMap[2] = &cart->m_chrData[banks[1] * 0x400];
		chrMap[3] = &cart->m_chrData[banks[1] * 0x400 + 0x400];
		chrMap[4] = &cart->m_chrData[banks[2] * 0x400];
		chrMap[5] = &cart->m_chrData[banks[3] * 0x400];
		chrMap[6] = &cart->m_chrData[banks[4] * 0x400];
		chrMap[7] = &cart->m_chrData[banks[5] * 0x400];
	}
}

void MMC3::updatePrgMapping() {
	uint8_t* last = &cart->m_prgRomData[(prgBank8kCount - 1) * 0x2000];
	uint8_t* secLast = &cart->m_prgRomData[(prgBank8kCount - 2) * 0x2000];

	if (prgMode == 0) {
		prgMap[0] = &cart->m_prgRomData[banks[6] * 0x2000];  // 8000
		prgMap[1] = &cart->m_prgRomData[banks[7] * 0x2000];  // A000
		prgMap[2] = secLast;                                 // C000
		prgMap[3] = last;                                    // E000
	}
	else {
		prgMap[0] = secLast;                                 // 8000
		prgMap[1] = &cart->m_prgRomData[banks[7] * 0x2000];  // A000
		prgMap[2] = &cart->m_prgRomData[banks[6] * 0x2000];  // C000
		prgMap[3] = last;                                    // E000
	}
}

void MMC3::writeRegister(uint16_t addr, uint8_t val, uint64_t currentCycle) {
	if (addr < 0x8000) {
		return;
	}
	else if (addr <= 0x9FFE && addr % 2 == 0) { // even
		// Control register
		prgMode = val & 0x40;
		chrMode = val & 0x80;
		m_regSelect = val & 0x7; // 3 LSB
		updatePrgMapping();
		updateChrMapping();
	}
	else if (addr <= 0x9FFF && addr % 2 == 1) { // odd
		banks[m_regSelect] = val;

		if (m_regSelect <= 1) {
			banks[m_regSelect] &= ~1;      // R0/R1: ignore bit 0
		}
		if (m_regSelect >= 6) {
			banks[m_regSelect] &= 0x3F;   // R6/R7: 6-bit
		}

		if (m_regSelect < 6) {
			banks[m_regSelect] &= chrBank1kCount - 1;
			updateChrMapping();
		}
		else {
			updatePrgMapping();
		}
	}
	// TODO Handle IRQ registers
}

void MMC3::recomputeMappings() {
	updatePrgMapping();
	updateChrMapping();
}

inline uint8_t MMC3::readCHR(uint16_t addr) const {
	return chrMap[(addr >> 10) & 7][addr & 0x3FF];
	// addr >> 10 = divide by 1024 -> index 0–7
	// addr & 0x3FF = offset within 1KB bank
}

void MMC3::writeCHR(uint16_t addr, uint8_t data) {
	if (!cart->isCHRWritable) {
		return; // Ignore writes if CHR is ROM
	}
	chrMap[(addr >> 10) & 7][addr & 0x3FF] = data;
}

inline uint8_t MMC3::readPRGROM(uint16_t addr) const {
	return prgMap[(addr >> 13) & 3][addr & 0x1FFF];
	// >>13 = divide by 8192 -> index 0–3
}

void MMC3::writePRGROM(uint16_t address, uint8_t data, uint64_t currentCycle) {
	if (address >= 0x8000) {
		writeRegister(address, data, currentCycle);
	}
}