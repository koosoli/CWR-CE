#pragma once

#include <string.h>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon::Foundation
{
// Find the last path separator ('/' or '\\').
inline const char* findLastSep(const char* path)
{
    const char* a = strrchr(path, '/');
    const char* b = strrchr(path, '\\');
    if (!a)
        return b;
    if (!b)
        return a;
    return (a > b) ? a : b;
}
inline char* findLastSep(char* path)
{
    char* a = strrchr(path, '/');
    char* b = strrchr(path, '\\');
    if (!a)
        return b;
    if (!b)
        return a;
    return (a > b) ? a : b;
}

inline const char* GetFilenameExt(const char* w)
{ // short name with extension
    const char* nam = findLastSep(w);
    if (nam)
    {
        return nam + 1;
    }
    if (w[0] != 0 && w[1] == ':')
    {
        return w + 2;
    }
    return w;
}
inline char* GetFilenameExt(char* w)
{ // short name with extension
    char* nam = findLastSep(w);
    if (nam)
    {
        return nam + 1;
    }
    if (w[0] != 0 && w[1] == ':')
    {
        return w + 2;
    }
    return w;
}

inline const char* GetFileExt(const char* n)
{ // extension including leading point
    const char* nam = strrchr(n, '.');
    if (nam)
    {
        return nam;
    }
    return n + strlen(n);
}
inline char* GetFileExt(char* n)
{ // extension including leading point
    char* nam = strrchr(n, '.');
    if (nam)
    {
        return nam;
    }
    return n + strlen(n);
}

inline void TerminateBy(char* s, int c)
{ // if c is not last character, add it
    char* e = s + strlen(s);
    if (e <= s || e[-1] != c)
    {
        *e++ = c, *e = 0;
    }
}

inline void GetFilename(char* dst, const char* sname)
{ // short name, no extension
    const char* name = findLastSep(sname);
    if (name)
    {
        name++;
    }
    else
    {
        name = sname;
    }
    ::strcpy(dst, name);
    char* ext = strchr(dst, '.');
    if (ext)
    {
        *ext = 0;
    }
}

inline bool IsRelativePath(const char* path)
{
    return (*path != '\\' && *path != '/' && !strstr(path, "..") && !strchr(path, ':'));
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::findLastSep;
using ::Poseidon::Foundation::GetFilenameExt;
using ::Poseidon::Foundation::GetFileExt;
using ::Poseidon::Foundation::TerminateBy;
using ::Poseidon::Foundation::GetFilename;
using ::Poseidon::Foundation::IsRelativePath;
