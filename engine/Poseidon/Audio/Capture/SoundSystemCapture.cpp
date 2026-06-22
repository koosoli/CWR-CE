#include <Poseidon/Audio/Capture/SoundSystemCapture.hpp>
#include <Poseidon/Audio/Capture/WaveCapture.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

IWave* SoundSystemCapture::CreateEmptyWave(float duration)
{
    auto* w = new WaveCapture("", duration);
    _waves.push_back(w);
    return w;
}

IWave* SoundSystemCapture::CreateWave(const char* filename, bool is3D, bool prealloc)
{
    auto* w = new WaveCapture(filename, 1.f);
    w->Set3D(is3D);
    _waves.push_back(w);
    return w;
}

void SoundSystemCapture::SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up)
{
    _listenerPos = pos;
    _listenerVel = vel;
    _listenerDir = dir;
    _listenerUp = up;
}

void SoundSystemCapture::Commit()
{
    for (auto* w : _waves)
    {
        if (!w->IsStopped())
        {
            w->SetVolumeAdjust(_waveVolume);
            w->RenderSamples(1);
        }
    }
}

void SoundSystemCapture::RenderAll(int sampleCount)
{
    for (int i = 0; i < sampleCount; ++i)
        Commit();
}

} // namespace Poseidon
