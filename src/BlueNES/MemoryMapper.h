#pragma once
#include <cstdint>

class MemoryMapper
{
public:
	virtual ~MemoryMapper() = default;
	virtual inline uint8_t read(uint16_t address) = 0;
	virtual inline void write(uint16_t address, uint8_t value) = 0;
};