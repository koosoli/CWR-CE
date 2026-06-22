#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>


namespace Poseidon
{
class IVoiceCaptureBackend;

class VoNCapture
{
  public:
    VoNCapture();
    ~VoNCapture();

    VoNCapture(const VoNCapture&) = delete;
    VoNCapture& operator=(const VoNCapture&) = delete;

    bool open(const char* deviceName, int sampleRate, int bufferSamples);
    void close();

    void start();
    void stop();

    int availableSamples() const;
    int read(int16_t* buffer, int maxSamples);

    float lastFramePeak() const;

    bool isOpen() const;
    bool isCapturing() const;
    int sampleRate() const;

    static std::vector<std::string> listDevices();

  private:
    IVoiceCaptureBackend* EnsureImpl() const;

    mutable std::unique_ptr<IVoiceCaptureBackend> _impl;
};

} // namespace Poseidon
