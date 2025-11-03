#pragma once
#include "IntegrationTest.h"

class ScrollTest : public IntegrationTest {
public:
    void Setup() override;
    void Update() override;
    bool IsComplete() override { return m_frameCount >= m_maxFrames; }
    bool WasSuccessful() override { return m_success; }
    std::string GetName() const override { return "Vertical Scroll Test"; }

private:
    int m_frameCount = 0;
    const int m_maxFrames = 300;
    bool m_success = false;
    int m_scrollY = 0;
    uint8_t m_ppuCtrl = 0x00;
};