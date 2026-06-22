#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{
// Logs every IWave call instead of producing audio — for inspecting the
// engine's audio call patterns without sound output.
class WaveText : public IWave
{
private:
	float _volume;
	float _length;
	Vector3 _position;
	Vector3 _velocity;
	bool _playing;
	bool _paused;
	bool _terminated;
	bool _everPlayed;
	bool _is3D;
	WaveKind _kind;
	float _accommodation;
	bool _accommodationEnabled;

public:
	explicit WaveText(RString name, float length = 1.0f);
	virtual ~WaveText();

	// Playback control
	virtual void Queue(IWave* wave, int repeat = 1) override;
	virtual void Repeat(int repeat = 1) override;
	virtual void Play() override;
	virtual void Stop() override;
	virtual void Pause() override;
	virtual void Resume() override;
	virtual void LastLoop() override;
	virtual void PlayUntilStopValue(float time) override;
	virtual void SetStopValue(float time) override;

	// Status queries
	virtual bool IsWaiting() override { return false; }
	virtual void Skip(float deltaT) override;
	virtual void Advance(float deltaT) override;
	virtual float GetLength() const override { return _length; }
	virtual bool IsStopped() override { return !_playing && !_paused; }
	virtual bool IsTerminated() override { return _terminated; }
	virtual void Restart() override;
	virtual WaveState State() override;

	// Volume control
	virtual bool IsMuted() const override { return false; }
	virtual float GetVolume() const override { return _volume; }
	virtual void SetVolume(float volume, float freq = 1.0f, bool immediate = false) override;
	virtual float GetFileMaxVolume() const override { return 1.0f; }
	virtual float GetFileAvgVolume() const override { return 0.5f; }

	// Accommodation
	virtual void SetAccommodation(float accom = 1.0f) override;
	virtual float GetAccommodation() const override { return _accommodation; }
	virtual void EnableAccommodation(bool enable) override;
	virtual bool AccommodationEnabled() const override { return _accommodationEnabled; }

	// Wave classification
	virtual void SetKind(WaveKind kind) override;
	virtual WaveKind GetKind() const override { return _kind; }

	// 3D positioning
	virtual void SetPosition(Vector3Par pos, Vector3Par vel, bool immediate = false) override;
	virtual Vector3 GetPosition() const override { return _position; }
	virtual void Set3D(bool is3D) override;
	virtual bool Get3D() const override { return _is3D; }
	virtual float Distance2D() const override { return 10.0f; }
};

} // namespace Poseidon
