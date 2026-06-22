#include <Poseidon/Audio/Capture/WaveCapture.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

WaveCapture::WaveCapture(RString name, float length) : IWave(name), _length(length) {}

void WaveCapture::Play()
{
    if (_terminated)
        return;
    _playing = true;
    _paused = false;
    _everPlayed = true;
}

void WaveCapture::Stop()
{
    // Stop -> StoppedReplayable, NOT Terminal (audio-invariants A-03).
    _playing = false;
    _paused = false;
}

void WaveCapture::Pause()
{
    if (_terminated || !_playing)
        return;
    _playing = false;
    _paused = true;
}

void WaveCapture::Resume()
{
    if (_terminated || !_paused)
        return;
    _playing = true;
    _paused = false;
}

WaveState WaveCapture::State()
{
    if (_terminated)
    {
        return WaveState::Terminal;
    }
    if (_paused)
    {
        return WaveState::Paused;
    }
    if (_playing)
    {
        return WaveState::Playing;
    }
    return _everPlayed ? WaveState::StoppedReplayable : WaveState::Created;
}

void WaveCapture::SetVolume(float volume, float freq, bool immediate)
{
    _volume = volume;
}

void WaveCapture::SetPosition(Vector3Par pos, Vector3Par vel, bool immediate)
{
    _position = pos;
    _velocity = vel;
}

float WaveCapture::GetEffectiveGain() const
{
    if (_is3D)
        return AudioMath::Gain3D(_volumeAdjust);
    return AudioMath::Gain2D(_volume, _accommodation, _accommodationEnabled, _volumeAdjust);
}

void WaveCapture::RenderSamples(int count)
{
    float gain = GetEffectiveGain();
    _capturedSamples.reserve(_capturedSamples.size() + count);
    for (int i = 0; i < count; ++i)
        _capturedSamples.push_back(gain);
}

} // namespace Poseidon
