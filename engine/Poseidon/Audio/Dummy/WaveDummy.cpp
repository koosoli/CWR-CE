#include <Poseidon/Audio/Dummy/WaveDummy.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

WaveDummy::WaveDummy(RString name, float length)
    : IWave(name), _volume(1.0f), _length(length), _position(VZero), _velocity(VZero), _playing(false), _paused(false),
      _terminated(false), _everPlayed(false), _is3D(true), _kind(WaveEffect), _accommodation(1.0f),
      _accommodationEnabled(true)
{
}

WaveDummy::~WaveDummy() = default;

void WaveDummy::Play()
{
    if (_terminated)
    {
        return;
    }
    _playing = true;
    _paused = false;
    _everPlayed = true;
}

void WaveDummy::Stop()
{
    // Stop -> StoppedReplayable.  Terminal is reached only via
    // LastLoop / natural EOF — audio-invariants A-03.
    _playing = false;
    _paused = false;
}

void WaveDummy::Pause()
{
    if (_terminated || !_playing)
    {
        return;
    }
    _playing = false;
    _paused = true;
}

void WaveDummy::Resume()
{
    if (_terminated || !_paused)
    {
        return;
    }
    _playing = true;
    _paused = false;
}

WaveState WaveDummy::State()
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

void WaveDummy::SetPosition(Vector3Par pos, Vector3Par vel, bool immediate)
{
    _position = pos;
    _velocity = vel;
}

} // namespace Poseidon
