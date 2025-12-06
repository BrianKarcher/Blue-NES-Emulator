#pragma once
#include "Nes.h"
#include "AudioBackend.h"
#include "SharedContext.h"

class EmulatorCore
{
public:
	EmulatorCore(SharedContext& ctx);
	~EmulatorCore();
	Nes nes;

	void Initialize();
	void loadROM(const char* filepath);
	void reset();
	void runFrame();
	void start();
	void stop();
	
private:
	SharedContext& context;
	AudioBackend audioBackend;
};