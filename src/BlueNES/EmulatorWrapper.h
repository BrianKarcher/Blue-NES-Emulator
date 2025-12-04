#pragma once
#include "Nes.h"

class EmulatorWrapper
{
public:
	Nes nes;

	void Initialize();
	void loadROM(const char* filepath);
	void start();
	void stop();
	void reset();
};