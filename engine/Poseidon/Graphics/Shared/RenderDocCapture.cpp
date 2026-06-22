#include <Poseidon/Graphics/Shared/RenderDocCapture.hpp>

#include <renderdoc/renderdoc_app.h>
#include <stdint.h>
#include <Poseidon/Foundation/Framework/Log.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <atomic>
#include <cstring>
#include <string>

namespace Poseidon
{
namespace
{
RENDERDOC_API_1_5_0* s_api = nullptr;
std::atomic<bool> s_initTried{false};
std::string s_lastPath;

bool DoInit()
{
#ifdef _WIN32
    // GetModuleHandle (not LoadLibrary) — only succeed if RenderDoc has
    // already injected its DLL into the process.  Loading it ourselves
    // would activate hooks mid-init and confuse the driver state we've
    // already brought up.
    HMODULE mod = GetModuleHandleA("renderdoc.dll");
    if (!mod)
        return false;
    auto getApi = reinterpret_cast<pRENDERDOC_GetAPI>(GetProcAddress(mod, "RENDERDOC_GetAPI"));
    if (!getApi)
        return false;
    void* ptr = nullptr;
    if (getApi(eRENDERDOC_API_Version_1_5_0, &ptr) != 1 || !ptr)
        return false;
    s_api = static_cast<RENDERDOC_API_1_5_0*>(ptr);
    int maj = 0, min = 0, pat = 0;
    s_api->GetAPIVersion(&maj, &min, &pat);
    LOG_INFO(Graphics, "RenderDoc API attached: v{}.{}.{}", maj, min, pat);
    return true;
#else
    // Linux: RenderDoc injects librenderdoc.so via LD_PRELOAD.  RTLD_NOLOAD
    // only returns a handle if it's already in the process (mirrors the
    // Windows GetModuleHandle behaviour — we never load it ourselves).
    void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD);
    if (!mod)
        return false;
    auto getApi = reinterpret_cast<pRENDERDOC_GetAPI>(dlsym(mod, "RENDERDOC_GetAPI"));
    if (!getApi)
        return false;
    void* ptr = nullptr;
    if (getApi(eRENDERDOC_API_Version_1_5_0, &ptr) != 1 || !ptr)
        return false;
    s_api = static_cast<RENDERDOC_API_1_5_0*>(ptr);
    int maj = 0, min = 0, pat = 0;
    s_api->GetAPIVersion(&maj, &min, &pat);
    LOG_INFO(Graphics, "RenderDoc API attached: v{}.{}.{}", maj, min, pat);
    return true;
#endif
}
} // namespace

namespace RdcCapture
{
bool Init()
{
    bool expected = false;
    if (s_initTried.compare_exchange_strong(expected, true))
    {
        if (!DoInit())
            LOG_INFO(Graphics, "RenderDoc not loaded — capture API unavailable");
    }
    return s_api != nullptr;
}

bool Available()
{
    return s_api != nullptr;
}

void SetPathTemplate(const char* prefix)
{
    if (!s_api || !prefix)
        return;
    s_api->SetCaptureFilePathTemplate(prefix);
}

void Trigger()
{
    if (!s_api)
        return;
    s_api->TriggerCapture();
    LOG_INFO(Graphics, "RenderDoc: capture triggered (next swap)");
}

const char* LastCapturePath()
{
    if (!s_api)
        return "";
    uint32_t n = s_api->GetNumCaptures();
    if (n == 0)
        return "";
    uint32_t pathLen = 0;
    uint64_t ts = 0;
    // First call with null buffer returns required pathLen.
    s_api->GetCapture(n - 1, nullptr, &pathLen, &ts);
    if (pathLen == 0)
        return "";
    s_lastPath.resize(pathLen);
    s_api->GetCapture(n - 1, s_lastPath.data(), &pathLen, &ts);
    // Trim trailing NUL the API includes in pathLen.
    if (!s_lastPath.empty() && s_lastPath.back() == '\0')
        s_lastPath.pop_back();
    return s_lastPath.c_str();
}
} // namespace RdcCapture
} // namespace Poseidon
