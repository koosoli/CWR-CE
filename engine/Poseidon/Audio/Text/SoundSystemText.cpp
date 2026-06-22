#include <Poseidon/Audio/Text/SoundSystemText.hpp>
#include <Poseidon/Audio/Text/WaveText.hpp>
#include <Poseidon/Audio/AudioFactory.hpp>
#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Core/Global.hpp>
#include <fmt/format.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

SoundSystemText::SoundSystemText()
    : _waveVolume(5.0f), _speechVolume(5.0f), _cdVolume(5.0f), _listenerPos(VZero), _listenerVel(VZero),
      _listenerDir(VForward), _listenerUp(VUp), _hwAccelEnabled(false), _eaxEnabled(false), _active(false)
{
    LOG_DEBUG(Audio, "[AudioText] SoundSystemText initialized");
}

SoundSystemText::~SoundSystemText()
{
    LOG_DEBUG(Audio, "[AudioText] SoundSystemText destroyed");
}

void RegisterTextAudioBackend()
{
    static bool registered = false;
    if (registered)
        return;

    AudioFactory::Register(AudioBackendDescriptor{
        "text",
        "Text audio engine",
        -1,
        []() -> IAudioSystem* { return new SoundSystemText(); },
        nullptr,
    });
    registered = true;
}

float SoundSystemText::GetWaveDuration(const char* filename)
{
    LOG_DEBUG(Audio, "[AudioText] GetWaveDuration('{}') -> 1.0s (default)", filename);
    return 1.0f;
}

IWave* SoundSystemText::CreateEmptyWave(float duration)
{
    LOG_DEBUG(Audio, "[AudioText] CreateEmptyWave(duration={:.2f}ms)", duration);
    return new WaveText(RString("empty"), duration / 1000.0f);
}

IWave* SoundSystemText::CreateWave(const char* filename, bool is3D, bool prealloc)
{
    LOG_DEBUG(Audio, "[AudioText] CreateWave(filename='{}', is3D={}, prealloc={})", filename, is3D, prealloc);

    float duration = GetWaveDuration(filename);
    WaveText* wave = new WaveText(RString(filename), duration);
    wave->Set3D(is3D);

    return wave;
}

void SoundSystemText::SetListener(Vector3Par pos, Vector3Par vel, Vector3Par dir, Vector3Par up)
{
    LOG_DEBUG(Audio,
              "[AudioText] SetListener(pos=[%.1f,%.1f,%.1f], vel=[%.1f,%.1f,%.1f], dir=[%.2f,%.2f,%.2f], "
              "up=[%.2f,%.2f,%.2f])",
              pos.X(), pos.Y(), pos.Z(), vel.X(), vel.Y(), vel.Z(), dir.X(), dir.Y(), dir.Z(), up.X(), up.Y(), up.Z());

    _listenerPos = pos;
    _listenerVel = vel;
    _listenerDir = dir;
    _listenerUp = up;
}

void SoundSystemText::Commit()
{
    // Too verbose - only log periodically or on state change
    static int commitCount = 0;
    if ((commitCount++ % 100) == 0)
    {
        LOG_DEBUG(Audio, "[AudioText] Commit() - frame sync (every 100 frames)");
    }
}

void SoundSystemText::Activate(bool active)
{
    LOG_DEBUG(Audio, "[AudioText] Activate({})", active);
    _active = active;
}

void SoundSystemText::SetEnvironment(const SoundEnvironment& env)
{
    const char* envType = (env.type == SEPlain)       ? "Plain"
                          : (env.type == SECity)      ? "City"
                          : (env.type == SEForest)    ? "Forest"
                          : (env.type == SEMountains) ? "Mountains"
                                                      : "Room";
    LOG_DEBUG(Audio, "[AudioText] SetEnvironment(type={}, size={:.1f}, density={:.2f})", envType, env.size,
              env.density);
}

void SoundSystemText::SetCDVolume(float vol)
{
    LOG_DEBUG(Audio, "[AudioText] SetCDVolume({:.2f})", vol);
    _cdVolume = vol;
}

void SoundSystemText::SetWaveVolume(float vol)
{
    LOG_DEBUG(Audio, "[AudioText] SetWaveVolume({:.2f})", vol);
    _waveVolume = vol;
}

void SoundSystemText::SetSpeechVolume(float vol)
{
    LOG_DEBUG(Audio, "[AudioText] SetSpeechVolume({:.2f})", vol);
    _speechVolume = vol;
}

void SoundSystemText::StartPreview()
{
    LOG_DEBUG(Audio, "[AudioText] StartPreview()");
}

void SoundSystemText::TerminatePreview()
{
    LOG_DEBUG(Audio, "[AudioText] TerminatePreview()");
}

void SoundSystemText::FlushBank(QFBank* bank)
{
    LOG_DEBUG(Audio, "[AudioText] FlushBank(bank={})", fmt::ptr(bank));
}

bool SoundSystemText::EnableHWAccel(bool val)
{
    LOG_DEBUG(Audio, "[AudioText] EnableHWAccel({}) -> false (not supported)", val);
    _hwAccelEnabled = val;
    return false;
}

bool SoundSystemText::EnableEAX(bool val)
{
    LOG_DEBUG(Audio, "[AudioText] EnableEAX({}) -> false (not supported)", val);
    _eaxEnabled = val;
    return false;
}

void SoundSystemText::LoadConfig()
{
    LOG_DEBUG(Audio, "[AudioText] LoadConfig()");
}

void SoundSystemText::SaveConfig()
{
    LOG_DEBUG(Audio, "[AudioText] SaveConfig()");
}

} // namespace Poseidon
