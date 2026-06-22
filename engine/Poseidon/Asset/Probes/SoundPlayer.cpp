#include <Poseidon/Asset/Probes/SoundPlayer.hpp>
#include <Poseidon/Asset/Probes/AudioInit.hpp>

namespace Poseidon
{

SoundPlayer::SoundPlayer()
{
    _audio = CreateToolAudio();
}

SoundPlayer::~SoundPlayer()
{
    stop();
    delete _audio;
}

bool SoundPlayer::play(const char* path)
{
    stop();
    if (!_audio)
        return false;

    _wave = _audio->CreateWave(path, false);
    if (!_wave || _wave->GetLength() <= 0.0f)
    {
        _wave = nullptr;
        return false;
    }
    startWave();
    return true;
}

bool SoundPlayer::playMemory(const void* data, size_t size, const char* ext)
{
    stop();
    if (!_audio)
        return false;

    _wave = _audio->CreateWaveFromMemory(data, size, ext, false);
    if (!_wave || _wave->GetLength() <= 0.0f)
    {
        _wave = nullptr;
        return false;
    }
    startWave();
    return true;
}

void SoundPlayer::stop()
{
    if (_wave)
    {
        _wave->Stop();
        _wave = nullptr;
    }
    _duration = 0.0f;
}

bool SoundPlayer::isPlaying() const
{
    return _wave && !_wave->IsStopped() && !_wave->IsTerminated();
}

void SoundPlayer::startWave()
{
    _duration = _wave->GetLength();
    _wave->Repeat(1);
    _wave->Play();
    _audio->Commit();
}

bool SoundPlayer::enableEAX(bool val)
{
    return _audio ? _audio->EnableEAX(val) : false;
}

bool SoundPlayer::applyEFXByName(const char* presetName, float size)
{
    return _audio ? _audio->ApplyEFXByName(presetName, size) : false;
}

} // namespace Poseidon
