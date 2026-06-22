#include <Poseidon/Core/ModInstall.hpp>

#include <Poseidon/Core/ModCollection.hpp>

#include <cjson/cJSON.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

namespace Poseidon
{
std::string ModInstallDir(const std::string& modsRoot, const std::string& modId)
{
    return modsRoot + "/@" + modId;
}

std::string ModInstallDir(const std::string& modsRoot, const std::string& modId, const std::string& folderName)
{
    if (!folderName.empty())
        return modsRoot + "/" + folderName;
    return ModInstallDir(modsRoot, modId);
}

std::string FindInstalledModDir(const std::string& modsRoot, const std::string& modId)
{
    ModCollection mods;
    for (Mod& mod : ScanModsRoot(modsRoot, ModSource::Workshop))
        mods.Add(std::move(mod));
    const Mod* found = mods.Find(modId);
    return found != nullptr ? found->path : std::string();
}

std::string ReadInstalledModVersion(const std::string& modsRoot, const std::string& modId)
{
    namespace fs = std::filesystem;
    const std::string installDir = FindInstalledModDir(modsRoot, modId);
    if (installDir.empty())
        return {};
    const fs::path metadata = fs::path(installDir) / "mod.json";
    std::error_code ec;
    if (!fs::exists(metadata, ec))
    {
        return {};
    }

    std::ifstream in(metadata, std::ios::binary);
    if (!in)
    {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    const std::string text = buffer.str();

    cJSON* root = cJSON_Parse(text.c_str());
    std::string version;
    if (root != nullptr)
    {
        const cJSON* item = cJSON_GetObjectItemCaseSensitive(root, "version");
        if (cJSON_IsString(item) && item->valuestring != nullptr)
        {
            version = item->valuestring;
        }
        cJSON_Delete(root);
    }
    return version;
}

ModInstallStatus GetModInstallStatus(const std::string& modsRoot, const std::string& modId,
                                     const std::string& catalogVersion)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    if (FindInstalledModDir(modsRoot, modId).empty())
    {
        return ModInstallStatus::NotInstalled;
    }

    const std::string installed = ReadInstalledModVersion(modsRoot, modId);
    // Installed with no readable version → can't prove it's stale, so don't nag.
    if (installed.empty() || installed == catalogVersion)
    {
        return ModInstallStatus::Installed;
    }
    return ModInstallStatus::UpdateAvailable;
}

std::vector<ScannedMod> ScanLocalMods(const std::string& modsRoot)
{
    // The catalog view of a local scan: ScanModsRoot does the work (and sorts by
    // display name); ScannedMod is the slimmer install-status projection of Mod.
    std::vector<ScannedMod> mods;
    for (const Mod& m : ScanModsRoot(modsRoot, ModSource::Local))
        mods.push_back({m.catalogId.empty() ? m.id : m.catalogId, m.id, m.name, m.version, m.sizeBytes});
    return mods;
}

bool WriteInstalledModManifest(const std::string& installDir, const std::string& modId, const std::string& name,
                               const std::string& version, const std::string& folderName,
                               const std::string& downloadUrl, int64_t sizeBytes, std::string* error)
{
    namespace fs = std::filesystem;
    const auto fail = [&](const std::string& message)
    {
        if (error != nullptr)
            *error = message;
        return false;
    };

    fs::create_directories(installDir);
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr)
        return fail("cannot allocate mod manifest");
    cJSON_AddStringToObject(root, "modId", modId.c_str());
    cJSON_AddStringToObject(root, "name", name.c_str());
    cJSON_AddStringToObject(root, "version", version.c_str());
    if (!folderName.empty())
        cJSON_AddStringToObject(root, "folderName", folderName.c_str());
    if (!downloadUrl.empty())
        cJSON_AddStringToObject(root, "downloadUrl", downloadUrl.c_str());
    if (sizeBytes > 0)
        cJSON_AddNumberToObject(root, "sizeBytes", static_cast<double>(sizeBytes));

    char* text = cJSON_Print(root);
    cJSON_Delete(root);
    if (text == nullptr)
        return fail("cannot serialize mod manifest");

    std::ofstream out(fs::path(installDir) / "mod.json", std::ios::binary);
    if (!out)
    {
        cJSON_free(text);
        return fail("cannot write mod manifest");
    }
    out << text;
    cJSON_free(text);
    return true;
}
} // namespace Poseidon
