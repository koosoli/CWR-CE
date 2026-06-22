#include <Poseidon/IO/Filesystem/FileOps.hpp>
#include <Poseidon/Foundation/platform.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <cstring>
#include <cstdio>
#include <stdint.h>
#include <strings.h>

// Walk each path component case-insensitively; write real-cased path into out.

namespace Poseidon
{
// Resolve the remaining components of `rel` (forward-slash, relative) under the already
// real-cased directory `base`, writing the full path into `out`.
//
// Backtracks: an exact-case match is tried first, but if descending into it fails to
// resolve the rest of the path, the case-insensitive siblings are tried too. This is
// required when a wrong-case sibling directory shadows the real one — e.g. an empty
// `missions/` next to the populated `Missions/`. Without backtracking the resolver
// commits to the exact-case (empty) match and never finds the file, which on Linux
// silently aborts the mission load (the single-mission launch builds a lowercase
// `missions\` path). The common case where every component matches exactly never opens
// a directory, so the fast path keeps its zero-overhead `stat`-only cost.
static bool ci_resolve_rec(const char* base, const char* rel, char* out, size_t outLen)
{
    while (*rel == '/')
        ++rel;
    if (*rel == 0)
    {
        snprintf(out, outLen, "%s", base);
        return true;
    }

    const char* slash = strchr(rel, '/');
    const size_t compLen = slash ? (size_t)(slash - rel) : strlen(rel);
    char component[MaxFileName];
    snprintf(component, sizeof(component), "%.*s", (int)compLen, rel);
    const char* rest = rel + compLen; // points at the trailing '/' or '\0'

    // Separator: avoid a double slash when `base` is the root "/".
    const char* sep = (base[0] && base[strlen(base) - 1] == '/') ? "" : "/";

    char candidate[MaxFileName];
    snprintf(candidate, sizeof(candidate), "%s%s%s", base, sep, component);
    struct stat st;
    if (stat(candidate, &st) == 0 && ci_resolve_rec(candidate, rest, out, outLen))
        return true;

    // Exact case missing or dead-ended — scan for a case-insensitive sibling.
    DIR* dir = opendir(base);
    if (!dir)
    {
        errno = ENOENT;
        return false;
    }

    bool resolved = false;
    struct dirent* entry;
    while ((entry = readdir(dir)))
    {
        if (strcmp(entry->d_name, component) == 0)
            continue; // already tried as the exact-case candidate
        if (strcasecmp(entry->d_name, component) == 0)
        {
            snprintf(candidate, sizeof(candidate), "%s%s%s", base, sep, entry->d_name);
            if (ci_resolve_rec(candidate, rest, out, outLen))
            {
                resolved = true;
                break;
            }
        }
    }
    closedir(dir);
    if (!resolved)
        errno = ENOENT;
    return resolved;
}

static bool ci_resolve_path(const char* path, char* out, size_t outLen)
{
    // Work on a mutable copy with forward slashes
    char work[MaxFileName];
    snprintf(work, sizeof(work), "%s", path);
    unixPath(work);

    // Absolute paths start from root "/", relative from ".".
    if (work[0] == '/')
        return ci_resolve_rec("/", work + 1, out, outLen);
    return ci_resolve_rec(".", work, out, outLen);
}

HANDLE OpenFileForRead(const char* path)
{
    LocalPath(fn, path); // backslash → forward slash via unixPath

    // Fast path: try exact name (zero overhead when case is already correct)
    int fd = ::open(fn, O_RDONLY);
    if (fd >= 0)
        return (HANDLE)(intptr_t)fd;

    if (errno != ENOENT)
        return INVALID_HANDLE_VALUE;

    // CI fallback: resolve each component with case-insensitive scan
    char resolved[MaxFileName];
    if (!ci_resolve_path(fn, resolved, sizeof(resolved)))
        return INVALID_HANDLE_VALUE;

    fd = ::open(resolved, O_RDONLY);
    return (fd >= 0) ? (HANDLE)(intptr_t)fd : INVALID_HANDLE_VALUE;
}

HANDLE OpenFileForWrite(const char* path, bool truncate)
{
    LocalPath(fn, path);
    int flags = O_WRONLY | O_CREAT | (truncate ? O_TRUNC : 0);
    int fd = ::open(fn, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return (fd >= 0) ? (HANDLE)(intptr_t)fd : INVALID_HANDLE_VALUE;
}

HANDLE OpenFileForAppend(const char* path)
{
    LocalPath(fn, path);
    int fd = ::open(fn, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    return (fd >= 0) ? (HANDLE)(intptr_t)fd : INVALID_HANDLE_VALUE;
}

int GetOpenFileSize(HANDLE f)
{
    struct stat st;
    if (fstat((int)(intptr_t)f, &st) != 0)
        return -1;
    return (int)st.st_size;
}

bool FilePathExists(const char* path)
{
    LocalPath(fn, path);
    struct stat st;
    if (stat(fn, &st) == 0)
        return true;
    if (errno != ENOENT)
        return false;
    char resolved[MaxFileName];
    return ci_resolve_path(fn, resolved, sizeof(resolved));
}

bool ResolveFilePath(const char* path, char* out, size_t outLen)
{
    LocalPath(fn, path);
    // Fast path: exact case already on disk — copy through, no opendir cost.
    struct stat st;
    if (stat(fn, &st) == 0)
    {
        snprintf(out, outLen, "%s", fn);
        return true;
    }
    if (errno != ENOENT)
        return false;
    return ci_resolve_path(fn, out, outLen);
}

} // namespace Poseidon
