#include "DebuggerContext.h"
#include <string>

void DebuggerContext::RequestStep() {

}

void DebuggerContext::RequestPause() {

}

void DebuggerContext::RequestContinue() {

}

bool DebuggerContext::ShouldPause(uint16_t pc) {
    std::lock_guard<std::mutex> lock(mutex);

    if (breakpoints[pc]) {
        paused = true;
    }

    if (paused) {
        if (stepRequested) {
            stepRequested = false;
            return false; // allow ONE instruction
        }
        return true;
    }

    return false;
}

DebuggerContext::CpuState DebuggerContext::GetCurrentState() {
	return lastState;
}

void DebuggerContext::OnInstructionExecuted(const DebuggerContext::CpuState& state,
    const std::string& disasm) {
    std::lock_guard<std::mutex> lock(mutex);
    log[state.pc] = disasm;
    lastState = state;
}