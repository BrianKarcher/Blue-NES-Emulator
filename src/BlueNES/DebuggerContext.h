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

    bool HasBreakpoint(uint16_t addr);

    //void RequestStep();
    //void RequestPause();
    //void RequestContinue();
    //bool ShouldPause(uint16_t pc);
    //CpuState GetCurrentState();
    //void OnInstructionExecuted(const CpuState& state,
    //    const std::string& disasm);

    std::mutex mutex;
    std::atomic<uint8_t> breakpoints[65536];
    //std::array<std::string, 0x10000> log{};
    std::atomic<bool> is_paused{ false };
    // Did the UI request a single step?
    std::atomic<bool> step_requested{ false };

    // Did the UI request a Step Over? (Auto-resume until PC hits target)
    std::atomic<bool> step_over_active{ false };
    std::atomic<uint16_t> step_over_target{ 0xFFFF };

    CpuState lastState{};
};