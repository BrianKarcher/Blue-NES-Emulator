#include "AudioBackend.h"

AudioBackend::AudioBackend()
    : m_xaudio2(nullptr)
    , m_masteringVoice(nullptr)
    , m_sourceVoice(nullptr)
    , m_voiceCallback(this)
    , m_sampleRate(44100)
    , m_channels(1)
    , m_initialized(false)
    , m_currentBuffer(0)
{
    for (int i = 0; i < BUFFER_COUNT; i++) {
        m_buffers[i].data.resize(SAMPLES_PER_BUFFER);
        m_buffers[i].inUse = false;
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
    if (!m_initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_queueMutex);

    // Add samples to queue
    for (size_t i = 0; i < count; i++) {
        m_sampleQueue.push(samples[i]);
    }

    // Try to submit buffers if we have enough samples
    ProcessAudioQueue();
}

size_t AudioBackend::GetQueuedSampleCount()
{
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_sampleQueue.size();
}

void AudioBackend::ProcessAudioQueue()
{
    // Check if we have enough samples and an available buffer
    while (m_sampleQueue.size() >= SAMPLES_PER_BUFFER) {
        // Find an available buffer
        bool foundBuffer = false;
        for (int i = 0; i < BUFFER_COUNT; i++) {
            if (!m_buffers[i].inUse) {
                m_currentBuffer = i;
                foundBuffer = true;
                break;
            }
        }

        if (!foundBuffer) {
            // All buffers are in use, wait for one to become available
            break;
        }

        // Fill the buffer
        AudioBuffer& buffer = m_buffers[m_currentBuffer];
        for (int i = 0; i < SAMPLES_PER_BUFFER; i++) {
            buffer.data[i] = m_sampleQueue.front();
            m_sampleQueue.pop();
        }

        // Submit the buffer
        SubmitBuffer();
    }
}

void AudioBackend::SubmitBuffer()
{
    AudioBuffer& buffer = m_buffers[m_currentBuffer];
    buffer.inUse = true;

    XAUDIO2_BUFFER xaudioBuffer = {};
    xaudioBuffer.AudioBytes = SAMPLES_PER_BUFFER * sizeof(float);
    xaudioBuffer.pAudioData = reinterpret_cast<const BYTE*>(buffer.data.data());
    xaudioBuffer.pContext = &m_buffers[m_currentBuffer]; // Pass buffer pointer as context
	xaudioBuffer.Flags = XAUDIO2_END_OF_STREAM;

    m_sourceVoice->SubmitSourceBuffer(&xaudioBuffer);
}

void AudioBackend::VoiceCallback::OnBufferEnd(void* pBufferContext)
{
    // Mark buffer as available
    if (pBufferContext) {
        AudioBuffer* buffer = static_cast<AudioBuffer*>(pBufferContext);
        buffer->inUse = false;

        // Try to process more samples
        std::lock_guard<std::mutex> lock(m_backend->m_queueMutex);
        m_backend->ProcessAudioQueue();
    }
}