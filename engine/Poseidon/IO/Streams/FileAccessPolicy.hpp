#pragma once

#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Foundation/Common/Win.h>

#include <cstddef>
#include <functional>

using Poseidon::IFileBuffer;

namespace QFileAccess
{

struct FileAccessHooks
{
    std::function<Ref<IFileBuffer>(const char* name)> openMappedFile;
    std::function<Ref<IFileBuffer>(const char* name)> openLoadedFile;
    std::function<bool(const char* name)> fileExists;
    std::function<Ref<IFileBuffer>(HANDLE file)> openMappedHandle;
    std::function<bool(size_t sizeNeeded)> freeUnusedBanks;
    std::function<void(const char* openName)> logMappingFailure;
};

bool MappingSupported();
const FileAccessHooks& DefaultFileAccessHooks();

Ref<IFileBuffer> OpenFileBufferAuto(const char* name, const FileAccessHooks* hooks = nullptr);
Ref<IFileBuffer> TryOpenMappedBankAccess(HANDLE file, size_t sizeNeeded, const char* openName,
                                         const FileAccessHooks* hooks = nullptr);

} // namespace QFileAccess

