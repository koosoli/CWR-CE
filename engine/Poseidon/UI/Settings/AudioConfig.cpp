#include <Poseidon/UI/Settings/AudioConfig.hpp>

#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

namespace
{
int ClampVolume(int v)
{
    return std::clamp(v, 0, 100);
}

bool DeviceListed(const std::string& name, const std::vector<std::string>& list)
{
    return std::find(list.begin(), list.end(), name) != list.end();
}
} // namespace

void AudioConfig::LoadDefaults()
{
    *this = AudioConfig{};
}

bool AudioConfig::Normalize(const Environment& env)
{
    bool changed = false;

    auto clampField = [&](int& v)
    {
        int c = ClampVolume(v);
        if (c != v)
        {
            v = c;
            changed = true;
        }
    };
    clampField(musicVolume);
    clampField(effectsVolume);
    clampField(speechVolume);

    // Device fields: empty string is always valid (system default).
    // A non-empty value must appear in the live device list.
    if (!outputDevice.empty() && !DeviceListed(outputDevice, env.ListOutputDevices()))
    {
        outputDevice.clear();
        changed = true;
    }
    if (!inputDevice.empty() && !DeviceListed(inputDevice, env.ListInputDevices()))
    {
        inputDevice.clear();
        changed = true;
    }

    return changed;
}

bool AudioConfig::Load(const std::string& path)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return false;

    ParamFile cfg;
    cfg.Parse(RString(path.c_str()));

    // Each field optional — partial files are tolerated, missing keys
    // keep the current in-memory value.  This makes forward-compat easy:
    // add a new field, default it in the class, old files still load.
    if (auto* e = cfg.FindEntry("musicVolume"))
        musicVolume = (int)*e;
    if (auto* e = cfg.FindEntry("effectsVolume"))
        effectsVolume = (int)*e;
    if (auto* e = cfg.FindEntry("speechVolume"))
        speechVolume = (int)*e;
    if (auto* e = cfg.FindEntry("eaxEnabled"))
        eaxEnabled = (bool)*e;
    if (auto* e = cfg.FindEntry("outputDevice"))
        outputDevice = ((RString)*e).Data();
    if (auto* e = cfg.FindEntry("inputDevice"))
        inputDevice = ((RString)*e).Data();

    return true;
}

bool AudioConfig::Save(const std::string& path) const
{
    std::error_code ec;
    std::filesystem::path p(path);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path(), ec);

    ParamFile cfg;
    cfg.Add("musicVolume", musicVolume);
    cfg.Add("effectsVolume", effectsVolume);
    cfg.Add("speechVolume", speechVolume);
    cfg.Add("eaxEnabled", eaxEnabled);
    cfg.Add("outputDevice", RString(outputDevice.c_str()));
    cfg.Add("inputDevice", RString(inputDevice.c_str()));

    cfg.Save(RString(path.c_str()));
    return std::filesystem::exists(path, ec);
}

} // namespace Poseidon
