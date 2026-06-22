#pragma once
#include <Poseidon/Audio/IAudioSystem.hpp>

namespace Poseidon
{

class SoundPlayer
{
  public:
    SoundPlayer();
    ~SoundPlayer();

    bool isReady() const { return _audio != nullptr; }

    bool play(const char* path);
    bool playMemory(const void* data, size_t size, const char* ext);
    void stop();

    bool       isPlaying() const;
    float      duration() const { return _duration; }
    IWave*     wave() const { return _wave; }
    IAudioSystem* audio() const { return _audio; }

    bool enableEAX(bool val);
    bool applyEFXByName(const char* presetName, float size);

  private:
    IAudioSystem* _audio    = nullptr;
    Ref<IWave>    _wave;
    float         _duration = 0.0f;

    void startWave();
};

} // namespace Poseidon
