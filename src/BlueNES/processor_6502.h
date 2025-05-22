#pragma once
#include <cstdint>

class Processor_6502
{
public:
	void Initialize(uint8_t* romData);
	void Run();
	uint8_t GetA();
	uint8_t GetX();
	uint8_t GetY();
};