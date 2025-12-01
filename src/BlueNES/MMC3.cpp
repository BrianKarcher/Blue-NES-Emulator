#include "MMC3.h"
#include "CPU.h"
#include "Cartridge.h"
#include "Bus.h"
#include "nes_ppu.h"
#include "RendererLoopy.h"

#define BANK_SIZE_CHR 0x400 // 1KB
#define BANK_SIZE_PRG 0x2000 // 8KB
static constexpr int A12_LOW_THRESHOLD = 8; // require A12 low >= this many PPU cycles

// ---------------- Debug helper ----------------
inline void MMC3::dbg(const wchar_t* fmt, ...) {
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

MMC3::MMC3(Bus* bus, uint8_t prgRomSize, uint8_t chrRomSize) {
	this->cpu = bus->cpu;
	this->bus = bus;
	renderLoopy = bus->ppu->renderer;
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

	irq_latch = 0;
	irq_counter = 0;
	irq_reload = false;
	irq_enabled = false;
	last_a12 = false;
	renderLoopy->setMapper(this);
	this->bus->ppu->setMapper(this);

	this->cart = bus->cart;
	uint32_t lastBankStart = (prgBank8kCount - 1) * BANK_SIZE_PRG;
	//lastPrg = &cartridge->m_prgRomData[lastBankStart];
	//secondLastPrg = &cartridge->m_prgRomData[lastBankStart - BANK_SIZE_PRG];
	updatePrgMapping();
	updateChrMapping();
}

MMC3::~MMC3() {
	renderLoopy->setMapper(nullptr);
	this->bus->ppu->setMapper(nullptr);
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
	dbg(L"(%d) 0x%04X 0x%02X (%d) \n", currentCycle, addr, val, val);
	switch (addr & 0xE001) {
		case 0x8000:
			// Control register
			prgMode = val & 0x40;
			chrMode = val & 0x80;
			m_regSelect = val & 0x7; // 3 LSB
			//dbg(L"prg: %d chr: %d reg: %d\n", prgMode, chrMode, m_regSelect);
			updatePrgMapping();
			updateChrMapping();
			break;

		case 0x8001:
			banks[m_regSelect] = val;
			if (m_regSelect == 2 && val == 0xC7) {
				int i = 0;
			}

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
			break;

		case 0xA000:
			cart->SetMirrorMode((val & 1) == 0 ? Cartridge::MirrorMode::VERTICAL : Cartridge::MirrorMode::HORIZONTAL);
			break;

		case 0xC000:  // IRQ Latch
			irq_latch = val;
			break;

		case 0xC001:  // IRQ Reload
			irq_reload = true;
			break;

		case 0xE000:  // IRQ Disable
			irq_enabled = false;
			acknowledgeIRQ();
			break;

		case 0xE001:  // IRQ Enable
			irq_enabled = true;
			break;
	}
}

void MMC3::triggerIRQ() {
	if (cpu) {
		cpu->setIRQ(true);
	}
}

void MMC3::acknowledgeIRQ() {
	if (cpu) {
		cpu->setIRQ(false);
	}
}

// Called by PPU when PPU address changes (A12 detection)
void MMC3::ClockIRQCounter(uint16_t ppu_address) {
	bool current_a12 = (ppu_address & 0x1000) != 0;

	// Track low-time duration
	if (!current_a12) {
		a12LowTime++;
	}
	else {
		// Detect rising edge of A12 (0 -> 1 transition)
		//if (!last_a12 && a12LowTime >= A12_LOW_THRESHOLD && current_a12) {
		if (!last_a12 && current_a12) {
			//dbg(L"Scanline (%d) detected, dec %d \n", bus->ppu->renderer->m_scanline, irq_counter);
			//if (a12_filter == 0) {
				// Clock the counter on rising edge
			if (irq_counter == 0 || irq_reload) {
				irq_counter = irq_latch;
				irq_reload = false;
			}
			else {
				irq_counter--;
			}

			// Trigger IRQ when counter reaches 0
			if (irq_counter == 0 && irq_enabled) {
				if (cpu) {
					dbg(L"IRQ Triggered at line %d\n", bus->ppu->renderer->m_scanline);
					cpu->setIRQ(true);
				}
			}

			//a12_filter = 6;  // Typical filter delay
		//}
		}
		a12LowTime = 0;
	}

	last_a12 = current_a12;

	//if (a12_filter > 0) {
	//	a12_filter--;
	//}
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