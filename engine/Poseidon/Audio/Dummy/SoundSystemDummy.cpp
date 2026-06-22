#include <Poseidon/Audio/Dummy/SoundSystemDummy.hpp>
#include <Poseidon/Audio/Dummy/WaveDummy.hpp>
#include <Poseidon/Audio/AudioFactory.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

SoundSystemDummy::SoundSystemDummy()
    : _waveVolume(5.0f), _speechVolume(5.0f), _cdVolume(5.0f), _listenerPos(VZero), _listenerVel(VZero),
      _listenerDir(VForward), _listenerUp(VUp), _hwAccelEnabled(false), _eaxEnabled(false)
{
    // No initialization needed for dummy system
}

SoundSystemDummy::~SoundSystemDummy()
{
    // No cleanup needed for dummy system
}

void RegisterDummyAudioBackend()
{
    static bool registered = false;
    if (registered)
        return;

    AudioFactory::Register(AudioBackendDescriptor{
        "dummy",
        "Dummy audio engine",
        0,
        []() -> IAudioSystem* { return new SoundSystemDummy(); },
        nullptr,
    });
    registered = true;
}

float SoundSystemDummy::GetWaveDuration(const char* filename)
{
    // Return a reasonable default duration (1 second)
    // Real implementation would parse the wave file header
    return 1.0f;
}

IWave* SoundSystemDummy::CreateEmptyWave(float duration)
{
    // Duration is in milliseconds, convert to seconds
    return new WaveDummy(RString("empty"), duration / 1000.0f);
}

IWave* SoundSystemDummy::CreateWave(const char* filename, bool is3D, bool prealloc)
{
    // Get estimated duration
    float duration = GetWaveDuration(filename);

    // Create dummy wave
    WaveDummy* wave = new WaveDummy(RString(filename), duration);
    wave->Set3D(is3D);

    return wave;
}

void SoundSystemDummy::SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up)
{
    // Store listener state but don't use it
    _listenerPos = pos;
    _listenerVel = vel;
    _listenerDir = dir;
    _listenerUp = up;
}

} // namespace Poseidon
