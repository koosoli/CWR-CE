#pragma once

#include <Poseidon/Audio/IAudioSystem.hpp>
#include <Poseidon/Audio/Shared/AudioMath.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <vector>


namespace Poseidon
{
// IWave that captures rendered PCM gain samples for tests.
class WaveCapture : public IWave
{
    float _volume = 1.f;
    float _length;
    Vector3 _position = {0, 0, 0};
    Vector3 _velocity = {0, 0, 0};
    bool _playing = false;
    bool _paused = false;
    bool _terminated = false;
    bool _everPlayed = false;
    bool _is3D = false;
    WaveKind _kind = WaveEffect;
    float _accommodation = 1.f;
    bool _accommodationEnabled = true;
    float _volumeAdjust = 1.f;
    std::vector<float> _capturedSamples;

public:
    explicit WaveCapture(RString name, float length = 1.f);
    ~WaveCapture() override = default;

    // Capture API (test-only)
    const std::vector<float>& GetCapturedSamples() const { return _capturedSamples; }
    void ClearCapture() { _capturedSamples.clear(); }
    float GetEffectiveGain() const;
    void SetVolumeAdjust(float adj) { _volumeAdjust = adj; }

    // Simulate rendering N samples at current gain
    void RenderSamples(int count);

    // IWave interface
    void Queue(IWave*, int) override {}
    void Repeat(int) override {}
    void Play() override;
    void Stop() override;
    void Pause() override;
    void Resume() override;
    void LastLoop() override { _terminated = true; }
    void PlayUntilStopValue(float) override {}
    void SetStopValue(float) override {}

    bool IsWaiting() override { return false; }
    void Skip(float) override {}
    void Advance(float) override {}
    float GetLength() const override { return _length; }
    bool IsStopped() override { return !_playing && !_paused; }
    bool IsTerminated() override { return _terminated; }
    void Restart() override
    {
        // Reset to a state where the next Play() succeeds.  Does NOT auto-play
        // — matches WaveOAL::Restart and audio-invariants A-04.
        _playing = false;
        _paused = false;
        _terminated = false;
        _everPlayed = true;
        _capturedSamples.clear();
    }
    WaveState State() override;

    bool IsMuted() const override { return false; }
    float GetVolume() const override { return _volume; }
    void SetVolume(float volume, float freq = 1.f, bool immediate = false) override;
    float GetFileMaxVolume() const override { return 1.f; }
    float GetFileAvgVolume() const override { return 0.5f; }

    void SetAccommodation(float accom = 1.f) override { _accommodation = accom; }
    float GetAccommodation() const override { return _accommodation; }
    void EnableAccommodation(bool e) override { _accommodationEnabled = e; }
    bool AccommodationEnabled() const override { return _accommodationEnabled; }

    void SetKind(WaveKind kind) override { _kind = kind; }
    WaveKind GetKind() const override { return _kind; }

    void SetPosition(Vector3Par pos, Vector3Par vel, bool immediate = false) override;
    Vector3 GetPosition() const override { return _position; }
    void Set3D(bool is3D) override { _is3D = is3D; }
    bool Get3D() const override { return _is3D; }
    float Distance2D() const override { return 10.f; }
};

} // namespace Poseidon
