#pragma once

#include <cstdint>
#include <array>
#include "MemoryMapper.h"

class APU;
class Bus;

class AudioMapper : public MemoryMapper {
public:
	APU& apu;

	AudioMapper(APU& apu) : apu(apu) {

	}

	~AudioMapper() {

	}

	inline uint8_t read(uint16_t address);

	inline void write(uint16_t address, uint8_t value);

	void register_memory(Bus& bus);
};