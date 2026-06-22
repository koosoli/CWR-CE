#pragma once

// Capture-to-playback sidetone driver.  Pulls 16 kHz mono PCM from a
// VoNCapture and queues it onto a dedicated OpenAL streaming source so
// the user hears their own mic with sub-200 ms latency.  Used by the
// Audio screen's Test Microphone modal; nothing else should hold one
// of these simultaneously since the AL source is exclusive.

#include <cstdint>
#include <vector>

#include <Poseidon/Audio/Voice/VoiceBackend.hpp>


namespace Poseidon
{
class MicLoopbackOpenAL : public IVoiceLoopbackBackend
{
  public:
    MicLoopbackOpenAL() = default;
    ~MicLoopbackOpenAL() override;

    MicLoopbackOpenAL(const MicLoopbackOpenAL&) = delete;
    MicLoopbackOpenAL& operator=(const MicLoopbackOpenAL&) = delete;

    // Allocate the AL source and buffer pool.  Returns false if AL
    // isn't ready or buffer/source allocation fails.  sampleRate must
    // match the capture's sample rate (typically 16 kHz).
    bool open(int sampleRate) override;
    void close() override;

    // Drain available samples from `capture`, queue them on the AL
    // source, and recycle finished buffers.  Call every frame while
    // the modal is mounted.
    void tick(VoNCapture& capture) override;

    bool isOpen() const override { return _source != 0; }

  private:
    static constexpr int kBufferCount      = 4;
    static constexpr int kSamplesPerBuffer = 1024;   // ~64 ms at 16 kHz

    unsigned int          _source = 0;
    std::vector<unsigned> _bufferPool;        // free buffer IDs
    int                   _sampleRate = 0;
};

} // namespace Poseidon
