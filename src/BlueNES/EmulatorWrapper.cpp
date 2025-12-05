#include "EmulatorWrapper.h"

EmulatorWrapper::EmulatorWrapper() {
    // Initialize audio backend
    if (!audioBackend.Initialize(44100, 1)) {  // 44.1kHz, mono
        // Handle error - audio failed to initialize
		throw std::runtime_error("Failed to initialize audio backend");
        //MessageBox(nullptr, L"Failed to initialize audio!", L"Error", MB_OK);
    }
}

EmulatorWrapper::~EmulatorWrapper() {
    audioBackend.Shutdown();
    nes.cart.unload();

    nes.input.CloseController();
}

void EmulatorWrapper::runFrame() {
	nes.input.PollControllerState();

	nes.cpu.cyclesThisFrame = 0;
    // Run PPU until frame complete (89342 cycles per frame)
	while (!nes.frameReady()) {
        nes.clock();
	}

    //OutputDebugStringW((L"CPU Cycles this frame: " + std::to_wstring(cpu.cyclesThisFrame) + L"\n").c_str());
    nes.cpu.nmiRequested = false;
    nes.ppu.setFrameComplete(false);

    // Submit the exact samples generated this frame
    // Check audio queue to prevent unbounded growth
    size_t queuedSamples = audioBackend.GetQueuedSampleCount();
    const size_t MAX_QUEUED_SAMPLES = 4410; // ~100ms of audio (44100 / 10)

    if (queuedSamples < MAX_QUEUED_SAMPLES) {
        audioBackend.SubmitSamples(nes.audioBuffer.data(), nes.audioBuffer.size());
        // Clear audio buffer for next frame
        nes.audioBuffer.clear();
    }
    else {
        // Audio queue is too large - skip this frame's audio to catch up
        //OutputDebugStringW(L"Audio queue overflow - dropping frame\n");
    }
}