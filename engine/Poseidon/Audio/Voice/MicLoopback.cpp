#include <Poseidon/Audio/Voice/MicLoopback.hpp>

#include <Poseidon/Audio/Voice/VoiceBackend.hpp>

namespace Poseidon
{
MicLoopback::MicLoopback() = default;

MicLoopback::~MicLoopback()
{
    close();
}

IVoiceLoopbackBackend* MicLoopback::EnsureImpl() const
{
    if (!_impl)
        _impl = GetSelectedVoiceBackend().createLoopback();
    return _impl.get();
}

bool MicLoopback::open(int sampleRate)
{
    return EnsureImpl()->open(sampleRate);
}

void MicLoopback::close()
{
    EnsureImpl()->close();
}

void MicLoopback::tick(VoNCapture& capture)
{
    EnsureImpl()->tick(capture);
}

bool MicLoopback::isOpen() const
{
    return EnsureImpl()->isOpen();
}

} // namespace Poseidon
