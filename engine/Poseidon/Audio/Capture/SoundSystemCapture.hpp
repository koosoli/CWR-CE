#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <vector>


namespace Poseidon
{
class WaveCapture;

// Test IAudioSystem that captures rendered output instead of playing it.
class SoundSystemCapture : public IAudioSystem
{
    float _waveVolume = 1.f;
    float _speechVolume = 1.f;
    float _cdVolume = 1.f;
    Vector3 _listenerPos = {0, 0, 0};
    Vector3 _listenerVel = {0, 0, 0};
    Vector3 _listenerDir = {0, 0, 1};
    Vector3 _listenerUp = {0, 1, 0};
    std::vector<WaveCapture*> _waves;

public:
    SoundSystemCapture() = default;
    ~SoundSystemCapture() override = default;

    // Test API
    const std::vector<WaveCapture*>& GetWaves() const { return _waves; }
    void RenderAll(int sampleCount);

    // IAudioSystem interface
    float GetWaveDuration(const char* filename) override { return 1.f; }
    IWave* CreateEmptyWave(float duration) override;
    IWave* CreateWave(const char* filename, bool is3D = true, bool prealloc = false) override;

    void SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up) override;
    Vector3 GetListenerPosition() const override { return _listenerPos; }
    void Commit() override;
    void Activate(bool) override {}
    void SetEnvironment(const SoundEnvironment&) override {}

    float GetCDVolume() const override { return _cdVolume; }
    void SetCDVolume(float vol) override { _cdVolume = vol; }
    void SetWaveVolume(float vol) override { _waveVolume = vol; }
    float GetWaveVolume() override { return _waveVolume; }
    void SetSpeechVolume(float vol) override { _speechVolume = vol; }
    float GetSpeechVolume() override { return _speechVolume; }

    void StartPreview() override {}
    void TerminatePreview() override {}
    void FlushBank(QFBank*) override {}

    bool EnableHWAccel(bool) override { return false; }
    bool EnableEAX(bool) override { return false; }
    bool GetEAX() const override { return false; }
    bool GetHWAccel() const override { return false; }
    void LoadConfig() override {}
    void SaveConfig() override {}
};

} // namespace Poseidon
