// EmulatorCore.cpp
#include "EmulatorCore.h"
#include "SharedContext.h"
#include <chrono>

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
    // Get high-resolution timer frequency once
    LARGE_INTEGER freq_li;
    if (!QueryPerformanceFrequency(&freq_li)) {
        return;
    }
    const long long freq = freq_li.QuadPart;

    using clock = std::chrono::high_resolution_clock;
    using namespace std::chrono;

    int frameCount = 0;

    // Target frame rate
    const double targetFps = 60.0;
    const long long ticksPerFrame = static_cast<long long>(freq / targetFps);

    LARGE_INTEGER fpsUpdateTime_li;
    QueryPerformanceCounter(&fpsUpdateTime_li);
	long long nextFpsUpdateTime = fpsUpdateTime_li.QuadPart;
    while (context.is_running) {
        // Frame start timestamp
        LARGE_INTEGER frameStart_li;
        QueryPerformanceCounter(&frameStart_li);
        long long frameStart = frameStart_li.QuadPart;

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
        frameCount++;

        // Measure time spent so far this frame
        LARGE_INTEGER frameEnd_li;
        QueryPerformanceCounter(&frameEnd_li);
        long long elapsedTicks = frameEnd_li.QuadPart - frameStart;


        //std::this_thread::sleep_for(milliseconds(500));
        if (elapsedTicks < ticksPerFrame) { // If more than 1ms to wait
            // Remaining ticks to wait to hit target frame time
            long long remainingTicks = ticksPerFrame - elapsedTicks;

            // Convert remaining ticks to milliseconds for Sleep()
            // Use integer math to avoid floating rounding issues.
            long long remainingMs = (remainingTicks * 1000) / freq;

            // Sleep for most of the remaining time (if enough to be worthwhile).
            // Subtract 1 ms to avoid oversleeping due to Sleep granularity.
            if (remainingMs > 1) {
                Sleep(static_cast<DWORD>(remainingMs - 1));
            }

            // Busy-wait the final tiny interval for precision
            while (true) {
                LARGE_INTEGER now_li;
                QueryPerformanceCounter(&now_li);
                if ((now_li.QuadPart - frameStart) >= ticksPerFrame) break;
                // Optionally call YieldProcessor() or std::this_thread::yield()
                // to avoid hammering the CPU too hard:
                YieldProcessor(); // on x86 this is a PAUSE; on MSVC resolves to intrinsic
            }
        }

        if (frameEnd_li.QuadPart >= nextFpsUpdateTime) {
			dbg(L"FPS: %d\n", frameCount);
            context.current_fps = frameCount;
            frameCount = 0;
            nextFpsUpdateTime = frameEnd_li.QuadPart + freq;
		}
	}
}

inline void EmulatorCore::dbg(const wchar_t* fmt, ...) {
#ifdef EMULATORCORE_DEBUG
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, sizeof(buf) / sizeof(buf[0]), _TRUNCATE, fmt, args);
    va_end(args);
    OutputDebugStringW(buf);
#endif
}

void EmulatorCore::runFrame() {
    // Ensure the audio buffer is clear before starting the frame
    nes.audioBuffer.clear();
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
    dbg(L"Cycles this frame %d, Samples this frame: %d\n", nes.cpu.cyclesThisFrame, nes.audioBuffer.size());
    if (!nes.audioBuffer.empty()) {
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