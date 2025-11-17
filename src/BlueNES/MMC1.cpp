#include "MMC1.h"

MMC1::MMC1() {
	// We use the 1 bit to track when the register is full, instead of a separate counter.
	shiftRegister = 0b10000;
}

void MMC1::writeRegister(uint16_t addr, uint8_t val) {
	if ((shiftRegister & 1) == 1) {
		// All five writes are in!
		shiftRegister >>= 1;
		// Record the last value that fills the register.
		// Record one bit at a time.
		shiftRegister |= ((val & 1) << 5);
		processShift(addr);
	}
	else {
		shiftRegister >>= 1;
		shiftRegister |= ((val & 1) << 5);
	}
}

void MMC1::processShift(uint16_t addr) {
	// Control register
	if (addr >= 0x8000 && addr <= 0x9FFF) {

	}
	// CHR Bank 0
	else if (addr >= 0xA000 && addr <= 0xBFFF) {

	}
	// CHR Bank 1
	else if (addr >= 0xC000 && addr <= 0xDFFF) {

	}
	// PRG Bank
	else if (addr >= 0xE000 && addr <= 0xFFFF) {

	}
}