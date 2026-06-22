// Multiplayer login identity. The client sends GetPublicKey() as identity.id; the
// server parses it with _atoi64 for ban-list and duplicate-identity checks. The
// identity is a system machine id (Windows MachineGuid, Linux /etc/machine-id),
// hashed to a stable decimal value — a practical non-Steam fallback identity, not
// a tamper-proof proof-of-ownership (a future Steam auth path is the trust root).
#include <Poseidon/Network/MultiplayerAuth.hpp>
#include <Poseidon/Foundation/platform.hpp>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <Poseidon/Foundation/Common/Win.h>
#else
#include <fstream>
#include <string>
#endif

namespace Poseidon
{
uint64_t MachineIdToPlayerId(const char* rawMachineId)
{
    if (!rawMachineId)
        return 0;

    // Trim surrounding whitespace (the /etc/machine-id file carries a trailing
    // newline; the id must be identical with or without it).
    const char* begin = rawMachineId;
    while (*begin != 0 && static_cast<unsigned char>(*begin) <= ' ')
        ++begin;
    const char* end = begin + strlen(begin);
    while (end > begin && static_cast<unsigned char>(*(end - 1)) <= ' ')
        --end;
    if (end == begin)
        return 0;

    // FNV-1a 64-bit over the trimmed bytes.
    uint64_t hash = 1469598103934665603ULL;
    for (const char* p = begin; p < end; ++p)
    {
        hash ^= static_cast<unsigned char>(*p);
        hash *= 1099511628211ULL;
    }

    // Mask to 63 bits so the value is a positive signed int64 (the _atoi64 /
    // decimal ban.txt contract). Steer clear of the server's 0/999 sentinels.
    hash &= 0x7FFFFFFFFFFFFFFFULL;
    if (hash == 0 || hash == 999)
        hash = 1;
    return hash;
}

RString ReadOsMachineId()
{
#ifdef _WIN32
    HKEY key;
    if (::RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &key) ==
        ERROR_SUCCESS)
    {
        char buffer[256];
        DWORD size = sizeof(buffer);
        DWORD type = 0;
        const LONG result =
            ::RegQueryValueEx(key, "MachineGuid", nullptr, &type, reinterpret_cast<LPBYTE>(buffer), &size);
        ::RegCloseKey(key);
        if (result == ERROR_SUCCESS && type == REG_SZ && size > 0)
        {
            buffer[size < sizeof(buffer) ? size : sizeof(buffer) - 1] = 0;
            return RString(buffer);
        }
    }
    return RString();
#else
    auto readFirstLine = [](const char* path) -> std::string
    {
        std::ifstream file(path);
        if (!file)
            return std::string();
        std::string line;
        std::getline(file, line);
        return line;
    };
    std::string id = readFirstLine("/etc/machine-id");
    if (id.empty())
        id = readFirstLine("/var/lib/dbus/machine-id");
    return RString(id.c_str());
#endif
}
} // namespace Poseidon

RString GetPublicKey()
{
    // Computed once: the machine id is stable for the process lifetime.
    static const RString cached = []() -> RString
    {
        const char* testMachineId = std::getenv("POSEIDON_TEST_MACHINE_ID");
        const RString raw = testMachineId && *testMachineId ? RString(testMachineId) : Poseidon::ReadOsMachineId();
        const uint64_t id =
            Poseidon::MachineIdToPlayerId(raw.GetLength() > 0 ? static_cast<const char*>(raw) : nullptr);
        if (id == 0)
            return RString("0");
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%" PRIu64, id);
        return RString(buffer);
    }();
    return cached;
}
