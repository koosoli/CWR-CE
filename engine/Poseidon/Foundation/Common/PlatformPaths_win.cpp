#include <Poseidon/Foundation/Common/PlatformPaths.hpp>
#include <cstdlib>
#include <sys/stat.h>
#include <ShlObj.h>
#include <direct.h>

namespace {

void ensureDirectory(const std::string& path) {
    if (path.empty()) return;
    for (size_t i = 1; i < path.size(); ++i) {
        if (path[i] == '/' || path[i] == '\\') {
            std::string partial = path.substr(0, i);
            _mkdir(partial.c_str());
        }
    }
    _mkdir(path.c_str());
}

std::string getWindowsFolder(int csidl, const char* appName) {
    char buf[MAX_PATH] = {};
    if (SUCCEEDED(SHGetFolderPathA(nullptr, csidl, nullptr, 0, buf))) {
        std::string dir = std::string(buf) + "\\" + appName;
        ensureDirectory(dir);
        return dir;
    }
    return std::string(".\\") + appName;
}

} // anonymous namespace

namespace Poseidon::Foundation {

std::string getUserConfigDir(const char* appName) {
    return getWindowsFolder(CSIDL_APPDATA, appName);
}

std::string getUserDataDir(const char* appName) {
    return getWindowsFolder(CSIDL_APPDATA, appName);
}

std::string getUserCacheDir(const char* appName) {
    return getWindowsFolder(CSIDL_LOCAL_APPDATA, appName);
}

std::string getUserDocumentsDir(const char* appName) {
    // CSIDL_PERSONAL = the user's Documents folder — discoverable and NOT roaming.
    return getWindowsFolder(CSIDL_PERSONAL, appName);
}

} // namespace Poseidon::Foundation

