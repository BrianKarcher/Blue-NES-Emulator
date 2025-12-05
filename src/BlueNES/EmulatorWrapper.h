#pragma once
#include "Nes.h"
#include "AudioBackend.h"

class EmulatorWrapper
{
public:
	EmulatorWrapper();
	~EmulatorWrapper();
	Nes nes;

	void Initialize();
	void loadROM(const char* filepath);
	void reset();
	void runFrame();
	void start();
	void stop();
	
private:
	AudioBackend audioBackend;
};