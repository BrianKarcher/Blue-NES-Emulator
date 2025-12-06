#pragma once
#include <mutex>
#include <cstdint>
#include <vector>

#define WIDTH 256
#define HEIGHT 240

class SharedContext {
private:
    std::mutex video_mutex;

    // We allocate the memory once.
    std::vector<uint32_t> buffer_1;
    std::vector<uint32_t> buffer_2;

    // Pointers that will swap targets
    uint32_t* p_front_buffer = nullptr; // UI Reads this
    uint32_t* p_back_buffer = nullptr;  // Core Writes this

public:
    // Lock-free atomic input (UI writes, Core reads)
    std::atomic<uint8_t> atomic_input{ 0 };
    std::atomic<bool> is_running{ true };

    SharedContext() {
        buffer_1.resize(WIDTH * HEIGHT, 0xFF000000); // Fill Black
        buffer_2.resize(WIDTH * HEIGHT, 0xFF000000);

        // Assign initial pointers
        p_front_buffer = buffer_1.data();
        p_back_buffer = buffer_2.data();
    }

    // --- CORE calls this ---
    // Returns a pointer to the memory where the Core should draw the NEXT frame.
    // We do not lock here to allow the core to write freely.
    uint32_t* GetBackBuffer() {
        return p_back_buffer;
    }

    // --- CORE calls this ---
    // Signals that the back buffer is full and ready to be shown.
    void SwapBuffers() {
        std::lock_guard<std::mutex> lock(video_mutex);
        std::swap(p_front_buffer, p_back_buffer);
    }

    // --- UI calls this ---
    // Returns the most recent completed frame. 
    // We return a const pointer because UI should only read.
    const uint32_t* GetFrontBuffer() {
        std::lock_guard<std::mutex> lock(video_mutex);
        return p_front_buffer;
    }
};