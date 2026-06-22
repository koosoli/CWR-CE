#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>

namespace Poseidon
{
// Logs every IAudioSystem call instead of producing audio — for inspecting
// the engine's audio call patterns without sound output.
class SoundSystemText : public IAudioSystem
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
	bool _active;

public:
	SoundSystemText();
	~SoundSystemText() override;

	// Wave creation
	float GetWaveDuration(const char* filename) override;
	IWave* CreateEmptyWave(float duration) override;
	IWave* CreateWave(const char* filename, bool is3D = true, bool prealloc = false) override;

	// 3D audio listener
	void SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up) override;
	Vector3 GetListenerPosition() const override { return _listenerPos; }
	void Commit() override;
	void Activate(bool active) override;
	void SetEnvironment(const SoundEnvironment& env) override;

	// Volume control
	float GetCDVolume() const override { return _cdVolume; }
	void SetCDVolume(float vol) override;
	void SetWaveVolume(float vol) override;
	float GetWaveVolume() override { return _waveVolume; }
	void SetSpeechVolume(float vol) override;
	float GetSpeechVolume() override { return _speechVolume; }

	// Preview mode
	void StartPreview() override;
	void TerminatePreview() override;

	// Bank flushing
	void FlushBank(QFBank* bank) override;

	// Hardware features
	bool EnableHWAccel(bool val) override;
	bool EnableEAX(bool val) override;
	bool GetEAX() const override { return _eaxEnabled; }
	bool GetHWAccel() const override { return _hwAccelEnabled; }

	// Config
	void LoadConfig() override;
	void SaveConfig() override;
};

} // namespace Poseidon
