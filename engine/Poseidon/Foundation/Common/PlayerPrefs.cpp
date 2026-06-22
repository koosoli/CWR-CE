#include <Poseidon/Foundation/Common/PlayerPrefs.hpp>
#include <Poseidon/Foundation/Common/PlatformPaths.hpp>
#include <cstdlib>
#include <fstream>
#include <unordered_map>
#include <utility>

namespace
{

std::string prefsFilePath(const char* appName)
{
    // Honour POSEIDON_USER_DIR so prefs.cfg sits with the profiles and other
    // user data (sandboxed/test runs set it). Without it, prefs would leak to
    // the real roaming appdata while profiles live under the override, leaving
    // the persisted profile name and the profile directories out of sync.
    const char* env = std::getenv("POSEIDON_USER_DIR");
    if (env != nullptr && env[0] != '\0')
        return std::string(env) + "/prefs.cfg";
    return Poseidon::Foundation::getUserConfigDir(appName) + "/prefs.cfg";
}

std::unordered_map<std::string, std::string> loadPrefs(const char* appName)
{
    std::unordered_map<std::string, std::string> prefs;
    std::ifstream file(prefsFilePath(appName));
    if (!file.is_open())
        return prefs;

    std::string line;
    while (std::getline(file, line))
    {
        auto eq = line.find('=');
        if (eq != std::string::npos && eq > 0)
        {
            prefs[line.substr(0, eq)] = line.substr(eq + 1);
        }
    }
    return prefs;
}

void savePrefs(const char* appName, const std::unordered_map<std::string, std::string>& prefs)
{
    std::ofstream file(prefsFilePath(appName));
    if (!file.is_open())
        return;

    for (auto& [key, value] : prefs)
    {
        file << key << '=' << value << '\n';
    }
}

} // anonymous namespace

namespace Poseidon::Foundation
{

std::string prefsGetString(const char* appName, const char* key, const char* defaultValue)
{
    auto prefs = loadPrefs(appName);
    auto it = prefs.find(key);
    if (it != prefs.end())
        return it->second;
    return defaultValue ? defaultValue : "";
}

void prefsSetString(const char* appName, const char* key, const char* value)
{
    auto prefs = loadPrefs(appName);
    prefs[key] = value ? value : "";
    savePrefs(appName, prefs);
}

} // namespace Poseidon::Foundation
