#include <Poseidon/Audio/Text/WaveText.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

WaveText::WaveText(RString name, float length)
    : IWave(name), _volume(1.0f), _length(length), _position(VZero), _velocity(VZero), _playing(false), _paused(false),
      _terminated(false), _everPlayed(false), _is3D(true), _kind(WaveEffect), _accommodation(1.0f),
      _accommodationEnabled(true)
{
    LOG_DEBUG(Audio, "[AudioText] Wave created: '{}', length={:.2f}s", (const char*)name, length);
}

WaveText::~WaveText()
{
    LOG_DEBUG(Audio, "[AudioText] Wave destroyed: '{}'", (const char*)Name());
}

void WaveText::Queue(IWave* wave, int repeat)
{
    LOG_DEBUG(Audio, "[AudioText] {}: Queue(wave='{}', repeat={})", (const char*)Name(),
              wave ? (const char*)wave->Name() : "null", repeat);
}

void WaveText::Repeat(int repeat)
{
    LOG_DEBUG(Audio, "[AudioText] {}: Repeat({})", (const char*)Name(), repeat);
}

void WaveText::Play()
{
    if (_terminated)
    {
        return;
    }
    LOG_DEBUG(Audio, "[AudioText] {}: Play() - started", (const char*)Name());
    _playing = true;
    _paused = false;
    _everPlayed = true;
}

void WaveText::Stop()
{
    LOG_DEBUG(Audio, "[AudioText] {}: Stop()", (const char*)Name());
    // audio-invariants A-03 — Stop -> StoppedReplayable.
    _playing = false;
    _paused = false;
}

void WaveText::Pause()
{
    if (_terminated || !_playing)
    {
        return;
    }
    LOG_DEBUG(Audio, "[AudioText] {}: Pause()", (const char*)Name());
    _playing = false;
    _paused = true;
}

void WaveText::Resume()
{
    if (_terminated || !_paused)
        return;
    LOG_DEBUG(Audio, "[AudioText] {}: Resume()", (const char*)Name());
    _playing = true;
    _paused = false;
}

void WaveText::LastLoop()
{
    LOG_DEBUG(Audio, "[AudioText] {}: LastLoop()", (const char*)Name());
    _terminated = true;
}

void WaveText::PlayUntilStopValue(float time)
{
    LOG_DEBUG(Audio, "[AudioText] {}: PlayUntilStopValue({:.2f})", (const char*)Name(), time);
}

void WaveText::SetStopValue(float time)
{
    LOG_DEBUG(Audio, "[AudioText] {}: SetStopValue({:.2f})", (const char*)Name(), time);
}

void WaveText::Skip(float deltaT)
{
    // Intentionally silent — logging this per-frame call would flood the log.
}

void WaveText::Advance(float deltaT)
{
    // Intentionally silent — logging this per-frame call would flood the log.
}

void WaveText::Restart()
{
    LOG_DEBUG(Audio, "[AudioText] {}: Restart()", (const char*)Name());
    // Reset to a state where the next Play() succeeds.  Does NOT auto-play —
    // matches WaveOAL::Restart and audio-invariants A-04.
    _playing = false;
    _paused = false;
    _terminated = false;
    _everPlayed = true;
}

WaveState WaveText::State()
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

void WaveText::SetVolume(float volume, float freq, bool immediate)
{
    LOG_DEBUG(Audio, "[AudioText] {}: SetVolume(vol={:.2f}, freq={:.2f}, immediate={})", (const char*)Name(), volume,
              freq, immediate);
    _volume = volume;
}

void WaveText::SetAccommodation(float accom)
{
    LOG_DEBUG(Audio, "[AudioText] {}: SetAccommodation({:.2f})", (const char*)Name(), accom);
    _accommodation = accom;
}

void WaveText::EnableAccommodation(bool enable)
{
    LOG_DEBUG(Audio, "[AudioText] {}: EnableAccommodation({})", (const char*)Name(), enable);
    _accommodationEnabled = enable;
}

void WaveText::SetKind(WaveKind kind)
{
    const char* kindStr = (kind == WaveEffect) ? "Effect" : (kind == WaveSpeech) ? "Speech" : "Music";
    LOG_DEBUG(Audio, "[AudioText] {}: SetKind({})", (const char*)Name(), kindStr);
    _kind = kind;
}

void WaveText::SetPosition(Vector3Par pos, Vector3Par vel, bool immediate)
{
    LOG_DEBUG(Audio,
              "[AudioText] {}: SetPosition(pos=[{:.1f},{:.1f},{:.1f}], vel=[{:.1f},{:.1f},{:.1f}], immediate={})",
              (const char*)Name(), pos.X(), pos.Y(), pos.Z(), vel.X(), vel.Y(), vel.Z(), immediate);
    _position = pos;
    _velocity = vel;
}

void WaveText::Set3D(bool is3D)
{
    LOG_DEBUG(Audio, "[AudioText] {}: Set3D({})", (const char*)Name(), is3D);
    _is3D = is3D;
}

} // namespace Poseidon
