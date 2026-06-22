#include <Poseidon/Audio/Voice/VonCapture.hpp>

#include <Poseidon/Audio/Voice/VoiceBackend.hpp>

namespace Poseidon
{
VoNCapture::VoNCapture() = default;
VoNCapture::~VoNCapture() = default;

IVoiceCaptureBackend* VoNCapture::EnsureImpl() const
{
    if (!_impl)
        _impl = GetSelectedVoiceBackend().createCapture();
    return _impl.get();
}

bool VoNCapture::open(const char* deviceName, int sampleRate, int bufferSamples)
{
    return EnsureImpl()->open(deviceName, sampleRate, bufferSamples);
}

void VoNCapture::close()
{
    EnsureImpl()->close();
}

void VoNCapture::start()
{
    EnsureImpl()->start();
}

void VoNCapture::stop()
{
    EnsureImpl()->stop();
}

int VoNCapture::availableSamples() const
{
    return EnsureImpl()->availableSamples();
}

int VoNCapture::read(int16_t* buffer, int maxSamples)
{
    return EnsureImpl()->read(buffer, maxSamples);
}

float VoNCapture::lastFramePeak() const
{
    return EnsureImpl()->lastFramePeak();
}

bool VoNCapture::isOpen() const
{
    return EnsureImpl()->isOpen();
}

bool VoNCapture::isCapturing() const
{
    return EnsureImpl()->isCapturing();
}

int VoNCapture::sampleRate() const
{
    return EnsureImpl()->sampleRate();
}

std::vector<std::string> VoNCapture::listDevices()
{
    const VoiceBackendDescriptor& backend = GetSelectedVoiceBackend();
    if (!backend.listDevices)
        return {};
    return backend.listDevices();
}

} // namespace Poseidon
