#pragma once

#include <Poseidon/Foundation/platform.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#endif

// Open a file for reading. On Linux, falls back to case-insensitive path walk on ENOENT.

namespace Poseidon
{
HANDLE OpenFileForRead(const char* path);

// Open (or create) a file for writing. Truncates if truncate=true.
HANDLE OpenFileForWrite(const char* path, bool truncate = true);

// Open (or create) a file for appending.
HANDLE OpenFileForAppend(const char* path);

// Return size of an open file in bytes, or -1 on error.
int GetOpenFileSize(HANDLE f);

// Check whether a file or directory exists. On Linux, case-insensitive path walk on ENOENT.
bool FilePathExists(const char* path);

// Resolve a path to its real on-disk form. On Linux, walks each component
// case-insensitively (so "Addons/foo.PBO" resolves to "addons/Foo.pbo" when that
// is what exists). Writes the resolved path into out and returns true; returns
// false if the path does not exist. On Windows the filesystem is case-insensitive,
// so this copies the input through when it exists.
bool ResolveFilePath(const char* path, char* out, size_t outLen);

} // namespace Poseidon

using Poseidon::OpenFileForRead;
using Poseidon::OpenFileForWrite;
using Poseidon::OpenFileForAppend;
using Poseidon::GetOpenFileSize;
using Poseidon::FilePathExists;
using Poseidon::ResolveFilePath;
