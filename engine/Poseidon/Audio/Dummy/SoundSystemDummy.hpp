#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{
// No-op IAudioSystem for dedicated-server / -nosound builds: every method is a
// no-op or returns a sensible default.
class SoundSystemDummy : public IAudioSystem
{
private:
	float _waveVolume;
	float _speechVolume;
	float _cdVolume;
	Vector3 _listenerPos;
	Vector3 _listenerVel;
	Vector3 _listenerDir;
	Vector3 _listenerUp;
	bool _hwAccelEnabled;
	bool _eaxEnabled;

public:
	SoundSystemDummy();
	~SoundSystemDummy() override;

	// Wave creation - returns dummy waves
	float GetWaveDuration(const char* filename) override;
	IWave* CreateEmptyWave(float duration) override;
	IWave* CreateWave(const char* filename, bool is3D = true, bool prealloc = false) override;

	// 3D audio listener - no-ops
	void SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up) override;
	Vector3 GetListenerPosition() const override { return _listenerPos; }
	void Commit() override {}
	void Activate(bool active) override {}
	void SetEnvironment(const SoundEnvironment& env) override {}

	// Volume control - stores values but doesn't use them
	float GetCDVolume() const override { return _cdVolume; }
	void SetCDVolume(float vol) override { _cdVolume = vol; }
	void SetWaveVolume(float vol) override { _waveVolume = vol; }
	float GetWaveVolume() override { return _waveVolume; }
	void SetSpeechVolume(float vol) override { _speechVolume = vol; }
	float GetSpeechVolume() override { return _speechVolume; }

	// Preview mode - no-ops
	void StartPreview() override {}
	void TerminatePreview() override {}

	// Bank flushing - no-op
	void FlushBank(QFBank* bank) override {}

	// Hardware features - always disabled
	bool EnableHWAccel(bool val) override { _hwAccelEnabled = val; return false; }
	bool EnableEAX(bool val) override { _eaxEnabled = val; return false; }
	bool GetEAX() const override { return false; }
	bool GetHWAccel() const override { return false; }

	// Config - no-ops
	void LoadConfig() override {}
	void SaveConfig() override {}
};

} // namespace Poseidon
