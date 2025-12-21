// EmulatorCore.cpp
#include "EmulatorCore.h"
#include "SharedContext.h"
#include "AudioRingBuffer.h"
#include <chrono>
#include "CPU.h"
#include "PPU.h"
#include "APU.h"
#include "Bus.h"
#include "Input.h"
#include <vector>
#include "Serializer.h"
#include <fstream>

EmulatorCore::EmulatorCore(SharedContext& ctx) : context(ctx), nes(ctx) {
    // Initialize audio backend
    if (!audioBackend.Initialize(44100, 1)) {  // 44.1kHz, mono
        // Handle error - audio failed to initialize
		throw std::runtime_error("Failed to initialize audio backend");
        //MessageBox(nullptr, L"Failed to initialize audio!", L"Error", MB_OK);
    }
    m_paused = true;

    // Set up DMC read callback
    nes.apu_->set_dmc_read_callback([this](uint16_t address) -> uint8_t {
        return nes.bus_->read(address);
    });

    // Get high-resolution timer frequency once
    LARGE_INTEGER freq_li;
    if (!QueryPerformanceFrequency(&freq_li)) {
        return;
    }
    freq = freq_li.QuadPart;

    // Target frame rate
    const double targetFps = 60.0;
    ticksPerFrame = static_cast<long long>(freq / targetFps);
}

EmulatorCore::~EmulatorCore() {
    audioBackend.Shutdown();
    nes.cart_->unload();

    nes.input_->CloseController();
}

void EmulatorCore::start() {
    core_thread = std::thread(&EmulatorCore::run, this);
}

void EmulatorCore::stop() {
    context.is_running = false;
    if (core_thread.joinable()) core_thread.join();
}

void EmulatorCore::run() {
    // Wait until a game is loaded.
    while (m_paused && context.is_running) {
        Sleep(1);
        processCommands();
    }

    LARGE_INTEGER fpsUpdateTime_li;
    QueryPerformanceCounter(&fpsUpdateTime_li);
	long long nextFpsUpdateTime = fpsUpdateTime_li.QuadPart;

    // Measure time spent so far this frame
    LARGE_INTEGER frameEnd_li;
    QueryPerformanceCounter(&frameEnd_li);
    nextFrameUpdateTime = frameEnd_li.QuadPart + ticksPerFrame;

    //using clock = std::chrono::high_resolution_clock;
    //using namespace std::chrono;

    int frameCount = 0;
    int audioCycleCounter = 0;

    while (context.is_running) {
        processCommands();
        
        if (m_paused) {
            Sleep(1);
            continue;
        }

        nes.input_->PollControllerState();
        audioCycleCounter += runFrame();
        context.SwapBuffers();
        frameCount++;

        // 1. Advance the target time *before* checking the difference
        //    This maintains the fixed-time step clock.
        //    If we are slow, this means nextFrameUpdateTime is now in the past.
        long long targetTime = nextFrameUpdateTime;
        nextFrameUpdateTime += ticksPerFrame;

        // 2. Get current time (actual time frame execution finished)
        LARGE_INTEGER currentFrame_li;
        QueryPerformanceCounter(&currentFrame_li);
        long long currentTime = currentFrame_li.QuadPart;

        // 3. Calculate remaining time (Wait time = Target Time - Current Time)
        //    If positive: we finished early, so wait.
        //    If zero/negative: we finished on time or late, so don't wait.
        long long time_to_wait_ticks = targetTime - currentTime;

#ifdef FPS_CAP
        //std::this_thread::sleep_for(milliseconds(500));
        if (time_to_wait_ticks > 0) { // If more than 1ms to wait

            // Convert remaining ticks to milliseconds for Sleep()
            // Use integer math to avoid floating rounding issues.
            long long remainingMs = (time_to_wait_ticks * 1000) / freq;

            // Sleep for most of the remaining time (if enough to be worthwhile).
            // Subtract 1 ms to avoid oversleeping due to Sleep granularity.
            if (remainingMs > 1) {
                Sleep(static_cast<DWORD>(remainingMs - 1));
            }

            // Busy-wait the final tiny interval for precision
            while (true) {
                LARGE_INTEGER now_li;
                QueryPerformanceCounter(&now_li);
                if ((now_li.QuadPart - nextFrameUpdateTime) >= ticksPerFrame) break;
                // Optionally call YieldProcessor() or std::this_thread::yield()
                // to avoid hammering the CPU too hard:
                YieldProcessor(); // on x86 this is a PAUSE; on MSVC resolves to intrinsic
            }
        }
#endif

        if (currentTime >= nextFpsUpdateTime) {
			// Update FPS once per second
			dbg(L"FPS: %d, cycles: %d\n", frameCount, audioCycleCounter);
            context.current_fps = frameCount;
            frameCount = 0;
            audioCycleCounter = 0;
            nextFpsUpdateTime += freq;
		}
	}
}

inline void EmulatorCore::processCommands() {
    CommandQueue::Command cmd;
    while (context.command_queue.TryPop(cmd)) {
        processCommand(cmd);
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

int EmulatorCore::runFrame() {
    // Ensure the audio buffer is clear before starting the frame
    nes.audioBuffer.clear();
	nes.cpu_->cyclesThisFrame = 0;
    nes.ppu_->setBuffer(context.GetBackBuffer());
    // Run PPU until frame complete (89342 cycles per frame)
	while (!nes.frameReady()) {
        nes.clock();
	}
    
    //OutputDebugStringW((L"CPU Cycles this frame: " + std::to_wstring(cpu.cyclesThisFrame) + L"\n").c_str());
    nes.ppu_->setFrameComplete(false);

    // Submit the exact samples generated this frame
    // Check audio queue to prevent unbounded growth
    size_t queuedSamples = audioBackend.GetQueuedSampleCount();
    const size_t MAX_QUEUED_SAMPLES = 4410; // ~100ms of audio (44100 / 10)
    while (nes.audioBuffer.size() < 735) {
        // Not enough samples generated - pad with last value
		nes.audioBuffer.push_back(nes.audioBuffer[nes.audioBuffer.size() - 1]);
    }
    dbg(L"Cycles this frame %d, Samples this frame: %d\n", nes.cpu_->cyclesThisFrame, nes.audioBuffer.size());
    int cycleCount = nes.audioBuffer.size();
    if (!nes.audioBuffer.empty()) {
        audioBackend.SubmitSamples(nes.audioBuffer.data(), nes.audioBuffer.size());
        // Clear audio buffer for next frame
        nes.audioBuffer.clear();
    }
    else {
        // Audio queue is too large - skip this frame's audio to catch up
        //OutputDebugStringW(L"Audio queue overflow - dropping frame\n");
    }
    return cycleCount;
}

void EmulatorCore::processCommand(const CommandQueue::Command& cmd) {
    switch (cmd.type) {
    case CommandQueue::CommandType::LOAD_ROM:
        audioBackend.resetBuffer();
        nes.cart_->unload();
        nes.ppu_->reset();
        nes.apu_->reset();
        nes.bus_->reset();
        nes.cart_->LoadROM(cmd.data);
        nes.cpu_->PowerOn();
        m_paused = false;
        UpdateNextFrameTime();
        // Set up DMC read callback
        nes.apu_->set_dmc_read_callback([this](uint16_t address) -> uint8_t {
            return nes.bus_->read(address);
        });
        break;
    case CommandQueue::CommandType::RESET:
        audioBackend.resetBuffer();
        nes.ppu_->reset();
        nes.apu_->reset();
        nes.bus_->reset();
        nes.cpu_->PowerOn();
        //m_core.Reset();
        break;
    case CommandQueue::CommandType::CLOSE:
        audioBackend.resetBuffer();
        nes.ppu_->reset();
        nes.apu_->reset();
        nes.bus_->reset();
        m_paused = true;
        // Set up DMC read callback
        nes.apu_->set_dmc_read_callback([this](uint16_t address) -> uint8_t {
            return nes.bus_->read(address);
        });
        break;
    case CommandQueue::CommandType::PAUSE:
        m_paused = true;
        break;
    case CommandQueue::CommandType::RESUME:
        m_paused = false;
        UpdateNextFrameTime();
        break;
    case CommandQueue::CommandType::STEP_FRAME:
        //if (m_paused) {
        //    m_core.StepFrame(m_frameBuffer.GetWriteBuffer(),
        //        m_audioBuffer,
        //        m_inputState);
        //    m_frameBuffer.SwapBuffers();
        //}
        break;
    case CommandQueue::CommandType::ADD_CONTROLLER:
        nes.input_->OpenFirstController();
		break;
    case CommandQueue::CommandType::REMOVE_CONTROLLER:
        nes.input_->CloseController();
		break;
    case CommandQueue::CommandType::SAVE_STATE:
        CreateSaveState();
        break;
    case CommandQueue::CommandType::LOAD_STATE:
        LoadState();
        break;
    }
}

void EmulatorCore::UpdateNextFrameTime() {
    LARGE_INTEGER frameEnd_li;
    QueryPerformanceCounter(&frameEnd_li);
    nextFrameUpdateTime = frameEnd_li.QuadPart + ticksPerFrame;
}

void EmulatorCore::CreateSaveState() {
    std::filesystem::path appFolder = nes.cart_->getAndEnsureSavePath();
    std::filesystem::path stateFilePath = appFolder / (nes.cart_->fileName + L".000");
    std::ofstream os(stateFilePath, std::ios::binary);
    Serializer serializer;
	serializer.StartSerialization(os);
    nes.Serialize(serializer);
    if (!os) {
        dbg(L"Failed to open save state file for writing: %s\n", stateFilePath.c_str());
        return;
	}
    //dbg(L"Save state created, size: %zu bytes\n", sizeof(SaveState));
}

void EmulatorCore::LoadState() {
    std::filesystem::path appFolder = nes.cart_->getAndEnsureSavePath();
    std::filesystem::path stateFilePath = appFolder / (nes.cart_->fileName + L".000");
    std::ifstream is(stateFilePath, std::ios::binary);
    if (!is) {
        dbg(L"Failed to open save state file for reading: %s\n", stateFilePath.c_str());
        return;
    }
    Serializer serializer;
    serializer.StartDeserialization(is);
    nes.Deserialize(serializer);
	//dbg(L"Save state loaded, size: %zu bytes\n", sizeof(SaveState));
}