#include <Poseidon/IO/Filesystem/FileOps.hpp>
#include <windows.h>

namespace Poseidon
{
HANDLE OpenFileForRead(const char* path)
{
    HANDLE h =
        ::CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, nullptr);
    return (h == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : h;
}

HANDLE OpenFileForWrite(const char* path, bool truncate)
{
    DWORD creation = truncate ? CREATE_ALWAYS : OPEN_ALWAYS;
    HANDLE h = ::CreateFileA(path, GENERIC_WRITE, 0, nullptr, creation, FILE_ATTRIBUTE_NORMAL, nullptr);
    return (h == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : h;
}

HANDLE OpenFileForAppend(const char* path)
{
    HANDLE h =
        ::CreateFileA(path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    return (h == INVALID_HANDLE_VALUE) ? INVALID_HANDLE_VALUE : h;
}

int GetOpenFileSize(HANDLE f)
{
    DWORD size = ::GetFileSize(f, nullptr);
    return (size == INVALID_FILE_SIZE) ? -1 : (int)size;
}

bool FilePathExists(const char* path)
{
    return ::GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

bool ResolveFilePath(const char* path, char* out, size_t outLen)
{
    // Windows' filesystem is case-insensitive; the path opens regardless of case.
    if (outLen == 0 || ::GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
        return false;
    ::lstrcpynA(out, path, (int)outLen); // copies up to outLen-1 chars and null-terminates
    return true;
}

} // namespace Poseidon
