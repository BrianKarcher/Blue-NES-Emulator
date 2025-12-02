// AudioBackend.h
#pragma once
#include <xaudio2.h>
#include <vector>
#include <queue>
#include <mutex>

#pragma comment(lib, "xaudio2.lib")

class AudioBackend {
public:
    AudioBackend();
    ~AudioBackend();

    bool Initialize(int sampleRate = 44100, int channels = 1);
    void AddBuffer(int buffer);
    void Shutdown();

    // Submit audio samples to the queue
    void SubmitSamples(const float* samples, size_t count);

    // Get the number of queued samples
    size_t GetQueuedSampleCount();

    // Check if audio is initialized
    bool IsInitialized() const { return m_initialized; }
    static const int SAMPLES_PER_BUFFER = 2048;

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

    void ProcessAudioQueue();
    void SubmitBuffer();

    IXAudio2* m_xaudio2;
    IXAudio2MasteringVoice* m_masteringVoice;
    IXAudio2SourceVoice* m_sourceVoice;
    VoiceCallback m_voiceCallback;

    int m_sampleRate;
    int m_channels;
    bool m_initialized;

    // Audio buffer management
    static const int BUFFER_COUNT = 3;

    struct AudioBuffer {
        std::vector<float> data;
        bool inUse = false;
    };

    AudioBuffer m_buffers[BUFFER_COUNT];
    int m_currentBuffer;

    // Sample queue
    std::queue<float> m_sampleQueue;
    std::mutex m_queueMutex;
};