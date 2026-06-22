#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{
// No-op IWave for dedicated-server / -nosound builds.
class WaveDummy : public IWave
{
private:
	float _volume;
	float _length;       // wave duration in seconds
	Vector3 _position;
	Vector3 _velocity;
	bool _playing;
	bool _paused;        // FSM Paused state (Pause/Resume preserve "had been playing" flag)
	bool _terminated;
	bool _everPlayed;    // tracks Created vs StoppedReplayable for State()
	bool _is3D;
	WaveKind _kind;
	float _accommodation;
	bool _accommodationEnabled;

public:
	explicit WaveDummy(RString name, float length = 1.0f);
	virtual ~WaveDummy();

	// Playback control - all no-ops
	virtual void Queue(IWave* wave, int repeat = 1) override {}
	virtual void Repeat(int repeat = 1) override {}
	virtual void Play() override;
	virtual void Stop() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void LastLoop() override { _terminated = true; }
	virtual void PlayUntilStopValue(float time) override {}
	virtual void SetStopValue(float time) override {}

	// Status queries
	virtual bool IsWaiting() override { return false; }
	virtual void Skip(float deltaT) override {}
	virtual void Advance(float deltaT) override {}
	virtual float GetLength() const override { return _length; }
	virtual bool IsStopped() override { return !_playing && !_paused; }
	virtual bool IsTerminated() override { return _terminated; }
	virtual void Restart() override
	{
		// Reset to a state where the next Play() succeeds.  Does NOT auto-play
		// — matches WaveOAL::Restart and audio-invariants A-04.  State() then
		// reports StoppedReplayable until Play() is called.
		_playing = false;
		_paused = false;
		_terminated = false;
		_everPlayed = true;
	}
	virtual WaveState State() override;

	// Volume control
	virtual bool IsMuted() const override { return false; }
	virtual float GetVolume() const override { return _volume; }
	virtual void SetVolume(float volume, float freq = 1.0f, bool immediate = false) override { _volume = volume; }
	virtual float GetFileMaxVolume() const override { return 1.0f; }
	virtual float GetFileAvgVolume() const override { return 0.5f; }

	// Accommodation (dynamic volume adjustment)
	virtual void SetAccommodation(float accom = 1.0f) override { _accommodation = accom; }
	virtual float GetAccommodation() const override { return _accommodation; }
	virtual void EnableAccommodation(bool enable) override { _accommodationEnabled = enable; }
	virtual bool AccommodationEnabled() const override { return _accommodationEnabled; }

	// Wave classification
	virtual void SetKind(WaveKind kind) override { _kind = kind; }
	virtual WaveKind GetKind() const override { return _kind; }

	// 3D positioning
	virtual void SetPosition(Vector3Par pos, Vector3Par vel, bool immediate = false) override;
	virtual Vector3 GetPosition() const override { return _position; }
	virtual void Set3D(bool is3D) override { _is3D = is3D; }
	virtual bool Get3D() const override { return _is3D; }
	virtual float Distance2D() const override { return 10.0f; } // arbitrary default
};

} // namespace Poseidon
