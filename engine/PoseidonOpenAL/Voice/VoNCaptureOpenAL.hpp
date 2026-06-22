#pragma once

#include <Poseidon/Audio/Voice/VoiceBackend.hpp>

#include <cstdint>
#include <string>
#include <vector>

// OpenAL C type — keep at global scope.
struct ALCdevice;

namespace Poseidon
{
class VoNCaptureOpenAL : public IVoiceCaptureBackend
{
  public:
    VoNCaptureOpenAL() = default;
    ~VoNCaptureOpenAL() override;

    VoNCaptureOpenAL(const VoNCaptureOpenAL&) = delete;
    VoNCaptureOpenAL& operator=(const VoNCaptureOpenAL&) = delete;

    // Open capture device. deviceName=nullptr for default.
    bool open(const char* deviceName, int sampleRate, int bufferSamples) override;
    void close() override;

    void start() override;
    void stop() override;

    int availableSamples() const override;
    int read(int16_t* buffer, int maxSamples) override;

    // Peak amplitude of the most recent read() call, normalised to
    // 0..1.  Returns 0 if read() hasn't been called this frame or
    // the device isn't open.  Used by the Audio screen's input-level
    // meter to give the user immediate "is my mic working" feedback
    // without going through the VoN encode + network path.
    float lastFramePeak() const override { return _lastFramePeak; }

    bool isOpen() const override { return _device != nullptr; }
    bool isCapturing() const override { return _capturing; }
    int sampleRate() const override { return _sampleRate; }

    static std::vector<std::string> ListDevices();

  private:
    ALCdevice* _device = nullptr;
    int _sampleRate = 0;
    bool _capturing = false;
    float _lastFramePeak = 0.0f;
};

} // namespace Poseidon
