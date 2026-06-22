#include <Poseidon/Core/Profile/ProfileManager.hpp>
#include <Poseidon/Core/Profile/UserConfig.hpp>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <system_error>
#include <utility>

namespace Poseidon
{
namespace fs = std::filesystem;

static std::string ensureTrailingSep(const std::string& path)
{
    if (path.empty())
        return path;
    if (path.back() != '/' && path.back() != '\\')
        return path + '/';
    return path;
}

static std::string usersDir(const std::string& basePath)
{
    return ensureTrailingSep(basePath) + "Users";
}

namespace ProfileManager
{

std::vector<ProfileInfo> EnumerateProfiles(const std::string& basePath)
{
    std::vector<ProfileInfo> profiles;
    std::string uDir = usersDir(basePath);

    std::error_code ec;
    if (!fs::is_directory(uDir, ec))
        return profiles;

    for (const auto& entry : fs::directory_iterator(uDir, ec))
    {
        if (!entry.is_directory())
            continue;

        std::string name = entry.path().filename().string();
        if (name.empty() || name[0] == '.')
            continue;
        if (IsServerProfileName(name))
            continue; // reserved dedicated-server profile, never user-selectable

        ProfileInfo info;
        info.name = name;
        info.path = ensureTrailingSep(entry.path().string());
        info.cfgPath = entry.path().string() + "/UserInfo.cfg";
        profiles.push_back(std::move(info));
    }

    std::sort(profiles.begin(), profiles.end(),
              [](const ProfileInfo& a, const ProfileInfo& b) { return a.name < b.name; });

    return profiles;
}

std::string GetProfileCfgPath(const std::string& basePath, const std::string& name)
{
    return usersDir(basePath) + "/" + name + "/UserInfo.cfg";
}

std::string GetProfileDirPath(const std::string& basePath, const std::string& name)
{
    return ensureTrailingSep(usersDir(basePath) + "/" + name);
}

bool CreateProfile(const std::string& basePath, const std::string& name)
{
    if (!IsValidProfileName(name))
        return false;

    std::string dirPath = usersDir(basePath) + "/" + name;

    std::error_code ec;
    if (fs::exists(dirPath, ec))
        return false; // already exists

    if (!fs::create_directories(dirPath, ec))
        return false;

    // Write default UserInfo.cfg
    std::string cfgPath = dirPath + "/UserInfo.cfg";
    UserConfig defaults;
    defaults.SaveToFile(cfgPath.c_str());
    return true;
}

bool DeleteProfile(const std::string& basePath, const std::string& name)
{
    if (!IsValidProfileName(name))
        return false;

    std::string dirPath = usersDir(basePath) + "/" + name;

    std::error_code ec;
    if (!fs::exists(dirPath, ec))
        return false;

    auto removed = fs::remove_all(dirPath, ec);
    return removed > 0 && !ec;
}

bool RenameProfile(const std::string& basePath, const std::string& oldName, const std::string& newName)
{
    if (!IsValidProfileName(oldName) || !IsValidProfileName(newName))
        return false;

    std::string oldPath = usersDir(basePath) + "/" + oldName;
    std::string newPath = usersDir(basePath) + "/" + newName;

    std::error_code ec;
    if (!fs::exists(oldPath, ec))
        return false;
    if (fs::exists(newPath, ec))
        return false; // target exists

    fs::rename(oldPath, newPath, ec);
    return !ec;
}

std::string CreateDefaultProfileIfNeeded(const std::string& basePath)
{
    auto profiles = EnumerateProfiles(basePath);
    if (!profiles.empty())
        return {};

    // Use OS username if valid, otherwise fall back to "Player"
    std::string defaultName = "Player";
    const char* user = std::getenv("USER");
    if (!user)
        user = std::getenv("USERNAME");
    if (user && IsValidProfileName(user))
        defaultName = user;

    if (CreateProfile(basePath, defaultName))
        return defaultName;
    return {};
}

bool IsValidProfileName(const std::string& name)
{
    if (name.empty())
        return false;
    if (name == "." || name == "..")
        return false;
    for (char c : name)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
            return false;
    }
    return true;
}

} // namespace ProfileManager
} // namespace Poseidon
