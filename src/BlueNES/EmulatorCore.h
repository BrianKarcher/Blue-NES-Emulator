// EmulatorCore.h
#pragma once
#include "Nes.h"
#include "AudioBackend.h"
#include "SharedContext.h"
#include <thread>

#define EMULATORCORE_DEBUG

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
	inline void dbg(const wchar_t* fmt, ...);
	std::thread core_thread;
	void run();
	Nes nes;
	SharedContext& context;
	AudioBackend audioBackend;
	void processCommand(const CommandQueue::Command& cmd);
	bool m_paused;
};