#include "Mapper.h"
#include "Bus.h"

uint8_t Mapper::read(uint16_t address) {
	if (address < 0x8000) {
		return m_prgRamData[address];
	}
	else {
		return readPRGROM(address); //m_prgRomData[address];
	}
}

void Mapper::write(uint16_t address, uint8_t value) {
	if (address < 0x8000) {
		m_prgRamData[address] = value;
	}
	else {
		writeRegister(address, value, 0);
	}
}

void Mapper::register_memory(Bus& bus) {
	//bus.registerAdd(0x4016, 0x4016, this);
	bus.registerAdd(0x4020, 0xFFFF, this);
}