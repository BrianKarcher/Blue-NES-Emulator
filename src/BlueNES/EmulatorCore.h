// EmulatorCore.h
#pragma once
#include "Nes.h"
#include "AudioBackend.h"
#include "SharedContext.h"
#include <thread>

//#define EMULATORCORE_DEBUG
#define FPS_CAP

class EmulatorCore
{
public:
	EmulatorCore(SharedContext& ctx);
	~EmulatorCore();

	void Initialize();
	void loadROM(const char* filepath);
	void reset();
	int runFrame();
	void start();
	void stop();
	
private:
	inline void dbg(const wchar_t* fmt, ...);
	inline void processCommands();
	std::thread core_thread;
	void run();
	Nes nes;
	SharedContext& context;
	AudioBackend audioBackend;
	void processCommand(const CommandQueue::Command& cmd);
	bool m_paused;
	long long nextFrameUpdateTime;
	long long ticksPerFrame;
	long long freq;
	void UpdateNextFrameTime();
};