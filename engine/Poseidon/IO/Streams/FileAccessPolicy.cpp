
#include <Poseidon/IO/Streams/FileAccessPolicy.hpp>

#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/Streams/FileMapping.hpp>
#include <Poseidon/IO/Filesystem/FileOps.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

using Poseidon::FileBufferLoaded;
using Poseidon::FileBufferMapped;
using Poseidon::FilePathExists;
using Poseidon::IFileBuffer;
using Poseidon::QFBank;

namespace QFileAccess
{

namespace
{

constexpr int kMaxBankMapRetries = 8;

Ref<IFileBuffer> OpenMappedFileDefault(const char* name)
{
    return Ref<IFileBuffer>(new FileBufferMapped(name));
}

Ref<IFileBuffer> OpenLoadedFileDefault(const char* name)
{
    return Ref<IFileBuffer>(new FileBufferLoaded(name));
}

Ref<IFileBuffer> OpenMappedHandleDefault(HANDLE file)
{
    return Ref<IFileBuffer>(new FileBufferMapped(file));
}

bool FileExistsDefault(const char* name)
{
    return FilePathExists(name);
}

bool FreeUnusedBanksDefault(size_t sizeNeeded)
{
    return QFBank::FreeUnusedBanks(sizeNeeded);
}

void LogMappingFailureDefault(const char* openName)
{
    RptF("Memory mapping of %s failed", openName);
}

} // namespace

bool MappingSupported()
{
#if defined(_WIN32) || !defined(NO_MMAP)
    return true;
#else
    return false;
#endif
}

const FileAccessHooks& DefaultFileAccessHooks()
{
    static const FileAccessHooks hooks{
        OpenMappedFileDefault,   OpenLoadedFileDefault,  FileExistsDefault,
        OpenMappedHandleDefault, FreeUnusedBanksDefault, LogMappingFailureDefault,
    };
    return hooks;
}

Ref<IFileBuffer> OpenFileBufferAuto(const char* name, const FileAccessHooks* hooks)
{
    const FileAccessHooks& activeHooks = hooks ? *hooks : DefaultFileAccessHooks();

    if (MappingSupported() && activeHooks.openMappedFile)
    {
        Ref<IFileBuffer> mapped = activeHooks.openMappedFile(name);
        if (mapped && !mapped->GetError())
        {
            return mapped;
        }

        if (activeHooks.fileExists && !activeHooks.fileExists(name))
        {
            return mapped;
        }
    }

    if (activeHooks.openLoadedFile)
    {
        return activeHooks.openLoadedFile(name);
    }

    return Ref<IFileBuffer>();
}

Ref<IFileBuffer> TryOpenMappedBankAccess(HANDLE file, size_t sizeNeeded, const char* openName,
                                         const FileAccessHooks* hooks)
{
    const FileAccessHooks& activeHooks = hooks ? *hooks : DefaultFileAccessHooks();

    if (!MappingSupported() || !activeHooks.openMappedHandle)
    {
        return Ref<IFileBuffer>();
    }

    int retryCount = 0;
    while (true)
    {
        Ref<IFileBuffer> mapped = activeHooks.openMappedHandle(file);
        if (mapped && !mapped->GetError())
        {
            return mapped;
        }

        if (activeHooks.freeUnusedBanks && activeHooks.freeUnusedBanks(sizeNeeded) && retryCount < kMaxBankMapRetries)
        {
            ++retryCount;
            continue;
        }

        if (activeHooks.logMappingFailure)
        {
            activeHooks.logMappingFailure(openName);
        }
        return Ref<IFileBuffer>();
    }
}

} // namespace QFileAccess
