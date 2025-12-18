#include "AudioMapper.h"
#include "Bus.h"
#include "APU.h"

void AudioMapper::register_memory(Bus& bus) {
	bus.registerAdd(0x4000, 0x4013, this);
	bus.registerAdd(0x4015, 0x4015, this);
	bus.registerAdd(0x4017, 0x4017, this);
}

inline uint8_t AudioMapper::read(uint16_t address) {
	return apu.read_register(address);
}

inline void AudioMapper::write(uint16_t address, uint8_t value) {
	apu.write_register(address, value);
}