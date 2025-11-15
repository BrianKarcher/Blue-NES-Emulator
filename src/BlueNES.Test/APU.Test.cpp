#include <cstdlib>
#include "pch.h"
#include "CppUnitTest.h"
#include "nes_apu.h"
#include "AudioBackend.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace APUTest
{
    TEST_CLASS(APUTest)
    {
    private:
        NES_APU apu;
        AudioBackend audioBackend;

    public:
        TEST_METHOD_INITIALIZE(TestSetup)
        {
            audioBackend.Initialize();
        }

        TEST_METHOD(TestAPUTone)
        {
            static const int TEST_FRAMES = 180; // ~3 seconds
            uint8_t volume = 0x0F;              // max volume

            // Setup pulse channel 1
            apu.write_register(0x4000, 0x30 | volume); // Duty 50%, constant volume
            apu.write_register(0x4002, 0xF7);          // Timer low byte
            apu.write_register(0x4003, 0x03);          // Timer high byte
            apu.write_register(0x4015, 0x01);          // Enable pulse 1

            // Reload the length counter so the pulse doesn't instantly die
            apu.reload_length_counter(0, 0x3F); // Channel 0, max length

            // NES CPU / APU constants
            const float CPU_FREQ = 1789773.0f;     // NTSC CPU freq
            const float SAMPLE_RATE = 44100.0f;    // Audio backend rate
            const float CPU_PER_SAMPLE = CPU_FREQ / SAMPLE_RATE;

            std::vector<float> buffer;
            float cpu_accum = 0.0f;

            for (int f = 0; f < TEST_FRAMES; f++)
            {
                int samples_per_frame = static_cast<int>(SAMPLE_RATE / 60); // 60 FPS
                for (int s = 0; s < samples_per_frame; s++)
                {
                    cpu_accum += CPU_PER_SAMPLE;
                    while (cpu_accum >= 1.0f)
                    {
                        apu.step();                 // Clock APU at CPU rate
                        apu.step_frame_sequencer(); // Advance envelopes / length / sweep
                        cpu_accum -= 1.0f;
                    }

                    float sample = apu.get_output();
                    buffer.push_back(sample);
                }

                // Submit audio for this frame
                audioBackend.SubmitSamples(buffer.data(), buffer.size());
                buffer.clear();
            }

            // Audible tone = pass
        }
    };
}
