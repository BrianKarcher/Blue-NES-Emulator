// AudioBackend.h
#pragma once
#include "AudioRingBuffer.h"
#include <xaudio2.h>
#include <vector>
#include <mutex>

#pragma comment(lib, "xaudio2.lib")

class AudioBackend {
public:
    AudioBackend();
    ~AudioBackend();

    bool Initialize(int sampleRate = 44100, int channels = 1);
    void Shutdown();

    // Submit audio samples (Producer side)
    void SubmitSamples(const float* samples, size_t count);

    // Get the number of queued samples
    size_t GetQueuedSampleCount() { return m_ringBuffer.GetAvailableRead(); }

    // Check if audio is initialized
    bool IsInitialized() const { return m_initialized; }

    // SAMPLES_PER_CHUNK is the size of the small XAudio2 chunks
    static const int SAMPLES_PER_CHUNK = 2048;
    // The total size of the circular buffer (Must be power of 2)
    static const int RING_BUFFER_CAPACITY = SAMPLES_PER_CHUNK * 8; // e.g., 16384 samples

    void resetBuffer() {
        m_ringBuffer.Reset();
	}

private:
    // XAudio2 voice callback
    class VoiceCallback : public IXAudio2VoiceCallback {
    public:
        VoiceCallback(AudioBackend* backend) : m_backend(backend) {}

        // Called when a buffer finishes playing
        void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;

        // Unused callbacks (required by interface)
        void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
        void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
        void STDMETHODCALLTYPE OnStreamEnd() override {}
        void STDMETHODCALLTYPE OnBufferStart(void*) override {}
        void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
        void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}

    private:
        AudioBackend* m_backend;
    };

    // Helper to submit the next contiguous chunk from the ring buffer
    void TrySubmitChunk();

    IXAudio2* m_xaudio2;
    IXAudio2MasteringVoice* m_masteringVoice;
    IXAudio2SourceVoice* m_sourceVoice;
    VoiceCallback m_voiceCallback;

    int m_sampleRate;
    int m_channels;
    bool m_initialized;

    // Audio buffer management
    AudioRingBuffer<float> m_ringBuffer; // The single continuous buffer

    // The fixed size XAudio2 submission chunks (used for context pointers)
    struct AudioChunk {
        int index; // Index used for tracking, or can be a chunk copy if ring buffer wasn't big enough
    };
    static const int CHUNK_COUNT = 3; // Number of chunks XAudio2 will have queued
    AudioChunk m_chunks[CHUNK_COUNT];
    int m_currentChunkIndex = 0;

    // No more m_queueMutex needed for sample submission! (Ring Buffer is lock-free)
    // A separate mutex for XAudio2 submission might still be required if called from multiple threads,
    // but typically the core only calls SubmitSamples, and the callback only calls TrySubmitChunk.
    std::mutex m_submissionMutex;
};