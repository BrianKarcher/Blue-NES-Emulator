#include "EmulatorCore.h"
#include "SharedContext.h"

EmulatorCore::EmulatorCore(SharedContext& ctx) : context(ctx), nes(ctx) {
    // Initialize audio backend
    if (!audioBackend.Initialize(44100, 1)) {  // 44.1kHz, mono
        // Handle error - audio failed to initialize
		throw std::runtime_error("Failed to initialize audio backend");
        //MessageBox(nullptr, L"Failed to initialize audio!", L"Error", MB_OK);
    }
    m_paused = true;

    // Set up DMC read callback
    nes.apu.set_dmc_read_callback([this](uint16_t address) -> uint8_t {
        return nes.bus.read(address);
    });
}

EmulatorCore::~EmulatorCore() {
    audioBackend.Shutdown();
    nes.cart.unload();

    nes.input.CloseController();
}

void EmulatorCore::start() {
    core_thread = std::thread(&EmulatorCore::run, this);
}

void EmulatorCore::stop() {
    context.is_running = false;
    if (core_thread.joinable()) core_thread.join();
}

void EmulatorCore::run() {
    using clock = std::chrono::high_resolution_clock;
    using namespace std::chrono;

    constexpr double TARGET_FRAME_TIME = 1.0 / 60.0; // 60 FPS
    auto lastFrameTime = clock::now();
    auto fpsUpdateTime = clock::now();
    int frameCount = 0;
    while (context.is_running) {
        CommandQueue::Command cmd;
        while (context.command_queue.TryPop(cmd)) {
            processCommand(cmd);
        }
        if (m_paused) {
            continue;
        }

        nes.input.PollControllerState();
        runFrame();
        context.SwapBuffers();

        // Frame timing
        auto frameEnd = clock::now();
        duration<double> frameDuration = frameEnd - lastFrameTime;
        lastFrameTime = frameEnd;

        // Sleep to maintain 60 FPS
        double sleepTime = TARGET_FRAME_TIME - frameDuration.count();
        if (sleepTime > 0) {
            std::this_thread::sleep_for(duration<double>(sleepTime));
        }
	}
}

void EmulatorCore::runFrame() {
	nes.cpu.cyclesThisFrame = 0;
    nes.ppu.setBuffer(context.GetBackBuffer());
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

void EmulatorCore::processCommand(const CommandQueue::Command& cmd) {
    switch (cmd.type) {
    case CommandQueue::CommandType::LOAD_ROM:
        nes.cart.unload();
        nes.ppu.reset();
        nes.bus.reset();
        nes.cart.LoadROM(cmd.data);
        nes.cpu.PowerOn();
        m_paused = false;
        break;
    case CommandQueue::CommandType::RESET:
        nes.ppu.reset();
        nes.bus.reset();
        nes.cpu.PowerOn();
        //m_core.Reset();
        break;
    case CommandQueue::CommandType::PAUSE:
        m_paused = true;
        break;
    case CommandQueue::CommandType::RESUME:
        m_paused = false;
        break;
    case CommandQueue::CommandType::STEP_FRAME:
        //if (m_paused) {
        //    m_core.StepFrame(m_frameBuffer.GetWriteBuffer(),
        //        m_audioBuffer,
        //        m_inputState);
        //    m_frameBuffer.SwapBuffers();
        //}
        break;
        // Handle other commands...
    }
}