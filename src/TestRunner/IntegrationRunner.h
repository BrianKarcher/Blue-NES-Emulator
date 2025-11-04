#pragma once
#include <malloc.h>
#include <memory.h>
#include <wchar.h>
#include <math.h>
#include <array>

class IntegrationTest;
class Core;

class IntegrationRunner
{
public:
	IntegrationRunner();
	void Initialize(Core* core);
	void RegisterTest(IntegrationTest* test);
	void RunTests();
	void Update();
	void PerformDMA();
	std::array<uint8_t, 0x100> oam = {};
	Core* GetCore() { return m_core; }

private:
	IntegrationTest* m_test;
	size_t m_currentTest = 0;
	Core* m_core;
};