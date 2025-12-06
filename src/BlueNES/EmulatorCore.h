#pragma once
#include "Nes.h"
#include "AudioBackend.h"
#include "SharedContext.h"

class EmulatorCore
{
public:
	EmulatorCore(SharedContext& ctx);
	~EmulatorCore();

	void Initialize();
	void loadROM(const char* filepath);
	void reset();
	void runFrame();
	void start();
	void stop();
	
private:
	void run();
	Nes nes;
	SharedContext& context;
	AudioBackend audioBackend;
	void processCommand(const CommandQueue::Command& cmd);
	bool m_paused;
};