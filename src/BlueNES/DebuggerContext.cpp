#include "DebuggerContext.h"
#include <string>

bool DebuggerContext::HasBreakpoint(uint16_t addr) {
    return breakpoints[addr].load(std::memory_order_relaxed) != 0;
}

void DebuggerContext::ToggleBreakpoint(uint16_t addr) {
    breakpoints[addr].store(~breakpoints[addr].load(std::memory_order_relaxed), std::memory_order_relaxed);
}

//void DebuggerContext::RequestStep() {
//
//}
//
//void DebuggerContext::RequestPause() {
//
//}
//
//void DebuggerContext::RequestContinue() {
//
//}

//bool DebuggerContext::ShouldPause(uint16_t pc) {
//    std::lock_guard<std::mutex> lock(mutex);
//
//    if (breakpoints[pc]) {
//        paused = true;
//    }
//
//    if (paused) {
//        if (stepRequested) {
//            stepRequested = false;
//            return false; // allow ONE instruction
//        }
//        return true;
//    }
//
//    return false;
//}

//DebuggerContext::CpuState DebuggerContext::GetCurrentState() {
//	return lastState;
//}

//void DebuggerContext::OnInstructionExecuted(const DebuggerContext::CpuState& state,
//    const std::string& disasm) {
//    std::lock_guard<std::mutex> lock(mutex);
//    log[state.pc] = disasm;
//    lastState = state;
//}