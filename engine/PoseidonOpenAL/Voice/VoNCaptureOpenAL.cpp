#include <PoseidonOpenAL/Voice/VoNCaptureOpenAL.hpp>
#include <PoseidonOpenAL/OpenALRuntime.hpp>


namespace Poseidon
{
VoNCaptureOpenAL::~VoNCaptureOpenAL()
{
    close();
}

bool VoNCaptureOpenAL::open(const char* deviceName, int sampleRate, int bufferSamples)
{
    close();
    if (!OpenALRuntime::EnsureLoaded())
        return false;

    _sampleRate = sampleRate;
    _device = alcCaptureOpenDevice(deviceName, sampleRate, AL_FORMAT_MONO16, bufferSamples);
    return _device != nullptr;
}

void VoNCaptureOpenAL::close()
{
    if (!_device)
        return;
    if (_capturing)
        stop();
    alcCaptureCloseDevice(_device);
    _device = nullptr;
}

void VoNCaptureOpenAL::start()
{
    if (!_device || _capturing)
        return;
    alcCaptureStart(_device);
    _capturing = true;
}

void VoNCaptureOpenAL::stop()
{
    if (!_device || !_capturing)
        return;
    alcCaptureStop(_device);
    _capturing = false;
}

int VoNCaptureOpenAL::availableSamples() const
{
    if (!_device)
        return 0;
    ALCint samples = 0;
    alcGetIntegerv(_device, ALC_CAPTURE_SAMPLES, 1, &samples);
    return samples;
}

int VoNCaptureOpenAL::read(int16_t* buffer, int maxSamples)
{
    if (!_device)
        return 0;
    int avail = availableSamples();
    int toRead = (avail < maxSamples) ? avail : maxSamples;
    if (toRead <= 0)
        return 0;
    alcCaptureSamples(_device, buffer, toRead);

    // Track the absolute peak of the read block, normalised to 0..1
    // so the Audio screen's level meter and the Test Microphone modal
    // can render a frame-rate-independent envelope.
    int16_t maxAbs = 0;
    for (int i = 0; i < toRead; ++i)
    {
        int16_t s = buffer[i];
        int16_t a = (s < 0) ? (int16_t)-s : s;
        if (a > maxAbs)
            maxAbs = a;
    }
    _lastFramePeak = (float)maxAbs / 32767.0f;

    return toRead;
}

std::vector<std::string> VoNCaptureOpenAL::ListDevices()
{
    std::vector<std::string> result;
    if (!OpenALRuntime::EnsureLoaded())
        return result;

    const ALCchar* list = alcGetString(nullptr, ALC_CAPTURE_DEVICE_SPECIFIER);
    if (!list)
        return result;
    while (*list)
    {
        result.emplace_back(list);
        list += result.back().size() + 1;
    }
    return result;
}

} // namespace Poseidon
