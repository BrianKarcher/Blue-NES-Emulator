#pragma once
#include <cstdint>
#include <mutex>
#include <array>
#include <string>

class DebuggerContext {
public:
    struct CpuState {
        uint16_t pc;
        uint8_t  a, x, y;
        uint8_t  sp;
        uint8_t  status;
        uint64_t cycle;
    };

    void RequestStep();
    void RequestPause();
    void RequestContinue();
    bool ShouldPause(uint16_t pc);
    CpuState GetCurrentState();
    void OnInstructionExecuted(const CpuState& state,
        const std::string& disasm);
private:
    std::mutex mutex;
    std::array<bool, 0x10000> breakpoints{};
    std::array<std::string, 0x10000> log{};
    bool paused = false;
    bool stepRequested = false;
    CpuState lastState{};
};