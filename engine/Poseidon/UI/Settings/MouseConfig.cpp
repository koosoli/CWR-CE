#include <Poseidon/UI/Settings/MouseConfig.hpp>

#include <Poseidon/IO/ParamFile/ParamFile.hpp>

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{

namespace
{
constexpr float kMinSensitivity = 0.5f;
constexpr float kMaxSensitivity = 2.0f;
} // namespace

void MouseConfig::LoadDefaults()
{
    *this = MouseConfig{};
}

bool MouseConfig::Normalize()
{
    bool changed = false;
    auto clamp = [&](float& v)
    {
        float c = std::clamp(v, kMinSensitivity, kMaxSensitivity);
        if (c != v)
        {
            v = c;
            changed = true;
        }
    };
    clamp(sensitivityX);
    clamp(sensitivityY);
    return changed;
}

bool MouseConfig::Load(const std::string& path)
{
    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return false;

    ParamFile cfg;
    cfg.Parse(RString(path.c_str()));

    if (auto* e = cfg.FindEntry("reverseY"))
        reverseY = (bool)*e;
    if (auto* e = cfg.FindEntry("buttonsReversed"))
        buttonsReversed = (bool)*e;
    if (auto* e = cfg.FindEntry("sensitivityX"))
        sensitivityX = (float)*e;
    if (auto* e = cfg.FindEntry("sensitivityY"))
        sensitivityY = (float)*e;

    return true;
}

bool MouseConfig::Save(const std::string& path) const
{
    std::error_code ec;
    std::filesystem::path p(path);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path(), ec);

    ParamFile cfg;
    cfg.Add("reverseY", reverseY);
    cfg.Add("buttonsReversed", buttonsReversed);
    cfg.Add("sensitivityX", sensitivityX);
    cfg.Add("sensitivityY", sensitivityY);

    cfg.Save(RString(path.c_str()));
    return std::filesystem::exists(path, ec);
}

} // namespace Poseidon
