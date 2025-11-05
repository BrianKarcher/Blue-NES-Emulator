#pragma once
#include "IntegrationTest.h"

class Core;
class IntegrationRunner;
class NesPPU;
class Bus;

class SimpleNESTest : public IntegrationTest {
public:
    void Setup(IntegrationRunner& runner) override;
    void Update() override;
    bool IsComplete() override { return m_frameCount >= m_maxFrames; }
    bool WasSuccessful() override { return m_success; }
    std::string GetName() const override { return "Simple NES Test"; }
private:
    int m_frameCount = 0;
    const int m_maxFrames = 3000;
    bool m_success = false;
    //uint8_t m_ppuCtrl = 0x00;
    //uint8_t* oam = nullptr;
    Core* m_core;
    Bus* bus;
    NesPPU* ppu;
    IntegrationRunner* m_runner;
};