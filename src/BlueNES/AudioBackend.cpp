#include "AudioBackend.h"
#include <mutex>

AudioBackend::AudioBackend()
    : m_xaudio2(nullptr), m_masteringVoice(nullptr), m_sourceVoice(nullptr)
    , m_voiceCallback(this), m_sampleRate(44100), m_channels(1)
    , m_initialized(false)
    , m_ringBuffer(RING_BUFFER_CAPACITY)
{
    // Initialize chunk contexts
    for (int i = 0; i < CHUNK_COUNT; i++) {
        m_chunks[i].index = i; // Assign simple index context
    }
}

AudioBackend::~AudioBackend()
{
    Shutdown();
}

bool AudioBackend::Initialize(int sampleRate, int channels)
{
    m_sampleRate = sampleRate;
    m_channels = channels;

    // Initialize COM (required for XAudio2)
    //CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    // Create XAudio2 instance
    HRESULT hr = XAudio2Create(&m_xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        return false;
    }

    // Create mastering voice
    hr = m_xaudio2->CreateMasteringVoice(&m_masteringVoice);
    if (FAILED(hr)) {
        m_xaudio2->Release();
        m_xaudio2 = nullptr;
        return false;
    }

    // Set up wave format
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    waveFormat.nChannels = m_channels;
    waveFormat.nSamplesPerSec = m_sampleRate;
    waveFormat.wBitsPerSample = 32; // 32-bit float
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    waveFormat.cbSize = 0;

    // Create source voice
    hr = m_xaudio2->CreateSourceVoice(&m_sourceVoice, &waveFormat, 0,
        XAUDIO2_DEFAULT_FREQ_RATIO, &m_voiceCallback);
    if (FAILED(hr)) {
        m_masteringVoice->DestroyVoice();
        m_xaudio2->Release();
        m_xaudio2 = nullptr;
        return false;
    }

    // Start the voice
    m_sourceVoice->Start(0);

    m_initialized = true;
    return true;
}

void AudioBackend::Shutdown()
{
    if (!m_initialized) {
        return;
    }

    if (m_sourceVoice) {
        m_sourceVoice->Stop(0);
        m_sourceVoice->DestroyVoice();
        m_sourceVoice = nullptr;
    }

    if (m_masteringVoice) {
        m_masteringVoice->DestroyVoice();
        m_masteringVoice = nullptr;
    }

    if (m_xaudio2) {
        m_xaudio2->Release();
        m_xaudio2 = nullptr;
    }

    CoUninitialize();
    m_initialized = false;
}

void AudioBackend::SubmitSamples(const float* samples, size_t count)
{
    if (!m_initialized) return;

    // --- Ring Buffer Write (Lock-Free) ---
    m_ringBuffer.Write(samples, count);

    // Try to submit chunks (Needs mutex if called concurrently with OnBufferEnd)
    std::lock_guard<std::mutex> lock(m_submissionMutex);
    TrySubmitChunk();
}

void AudioBackend::TrySubmitChunk()
{
    // Check how many buffers XAudio2 currently has queued
    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state);

    // Only queue if we have less than the max (CHUNK_COUNT) queued
    while (state.BuffersQueued < CHUNK_COUNT) {

        // ... (check for contiguous_samples remains the same) ...
        const float* chunk_ptr;
        size_t contiguous_samples = m_ringBuffer.ReadPointer(&chunk_ptr);

        if (contiguous_samples < SAMPLES_PER_CHUNK) {
            break;
        }

        // 2. Submit the buffer (Zero-Copy Submission)
        XAUDIO2_BUFFER xaudioBuffer = {};
        xaudioBuffer.AudioBytes = SAMPLES_PER_CHUNK * sizeof(float);
        xaudioBuffer.pAudioData = reinterpret_cast<const BYTE*>(chunk_ptr);

        // --- FIX: Use the cycling index to set the context pointer ---
        // Pass the actual pointer to the chunk structure as context
        xaudioBuffer.pContext = &m_chunks[m_currentChunkIndex];

        xaudioBuffer.Flags = 0;

        HRESULT hr = m_sourceVoice->SubmitSourceBuffer(&xaudioBuffer);
        if (FAILED(hr)) {
            break;
        }

        // Cycle the index for the next submission
        m_currentChunkIndex = (m_currentChunkIndex + 1) % CHUNK_COUNT;

        // 3. Advance the ring buffer's read pointer
        m_ringBuffer.AdvanceRead(SAMPLES_PER_CHUNK);

        // Update state for the next loop iteration
        m_sourceVoice->GetState(&state);
    }
}

void AudioBackend::VoiceCallback::OnBufferEnd(void* pBufferContext)
{
    // OnBufferEnd is called by an internal XAudio2 thread, so it must be thread-safe.
    std::lock_guard<std::mutex> lock(m_backend->m_submissionMutex);

    // NOTE: In the ring buffer model, we no longer need to free/mark specific chunks.
    // The ring buffer is always being advanced by the consumer (TrySubmitChunk).
    // The only action needed is to try and submit the next chunk.
    m_backend->TrySubmitChunk();
}