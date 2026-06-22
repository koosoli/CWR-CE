#pragma once

#include <Poseidon/Foundation/Strings/RString.hpp>

#ifdef _WIN32
#define VOICE_LANG_PATH_STRICMP _stricmp
#else
#include <strings.h>
#define VOICE_LANG_PATH_STRICMP strcasecmp
#endif

// Insert a language token before the file extension of a sound path.
//   "missions/Demo/sound/s02v01.ogg" + "Czech" → "missions/Demo/sound/s02v01.Czech.ogg"
//
// Returns an empty RString when:
//   - lang is empty
//   - path has no extension
//   - the last '.' is in a parent directory, not the basename
//
// Pure string transformation — caller decides whether the resulting file actually
// exists. Used by FindSound/FindRadio to prefer per-voice-language overrides.

namespace Poseidon
{
inline RString WithLangSuffix(RString path, RString lang)
{
    if (lang.GetLength() == 0 || path.GetLength() == 0)
        return RString();

    const char* p = (const char*)path;
    const int len = path.GetLength();

    int dotPos = -1;
    for (int i = len - 1; i >= 0; --i)
    {
        const char c = p[i];
        if (c == '.')
        {
            dotPos = i;
            break;
        }
        if (c == '/' || c == '\\')
            return RString();
    }
    if (dotPos < 0)
        return RString();

    char buf[1024];
    const int n = std::snprintf(buf, sizeof(buf), "%.*s.%s%s", dotPos, p, (const char*)lang, p + dotPos);
    if (n <= 0 || n >= (int)sizeof(buf))
        return RString();

    return RString(buf);
}

// Inverse of WithLangSuffix: if `path` ends with ".<lang>.<ext>" for the given lang,
// return the path without that ".<lang>" segment. Returns empty otherwise.
//   "missions/Demo/sound/s02v01.Czech.lip" + "Czech" → "missions/Demo/sound/s02v01.lip"
//   "missions/Demo/sound/s02v01.lip" + "Czech" → ""  (no suffix to strip)
inline RString WithoutLangSuffix(RString path, RString lang)
{
    if (lang.GetLength() == 0 || path.GetLength() == 0)
        return RString();

    const char* p = (const char*)path;
    const int len = path.GetLength();
    const int langLen = lang.GetLength();

    int extDot = -1;
    for (int i = len - 1; i >= 0; --i)
    {
        const char c = p[i];
        if (c == '.')
        {
            extDot = i;
            break;
        }
        if (c == '/' || c == '\\')
            return RString();
    }
    if (extDot < 0)
        return RString();

    int langDot = -1;
    for (int i = extDot - 1; i >= 0; --i)
    {
        const char c = p[i];
        if (c == '.')
        {
            langDot = i;
            break;
        }
        if (c == '/' || c == '\\')
            return RString();
    }
    if (langDot < 0)
        return RString();

    const int segLen = extDot - langDot - 1;
    if (segLen != langLen)
        return RString();

    char seg[64];
    if (segLen >= (int)sizeof(seg))
        return RString();
    std::memcpy(seg, p + langDot + 1, segLen);
    seg[segLen] = '\0';
    if (VOICE_LANG_PATH_STRICMP(seg, (const char*)lang) != 0)
        return RString();

    char buf[1024];
    const int n = std::snprintf(buf, sizeof(buf), "%.*s%s", langDot, p, p + extDot);
    if (n <= 0 || n >= (int)sizeof(buf))
        return RString();
    return RString(buf);
}

} // namespace Poseidon
