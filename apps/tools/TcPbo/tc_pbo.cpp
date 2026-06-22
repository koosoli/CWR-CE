#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <format>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

// The WCX API uses HANDLE as an opaque pointer (stores PboArchive*).
// On Linux, the engine defines HANDLE as int — too small for 64-bit pointers.
// We use a separate type for the plugin API handles.
#ifndef _WIN32
#define WCX_HANDLE intptr_t
#else
#define WCX_HANDLE HANDLE
#endif

#include "wcxhead.h"
#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>
#include <Poseidon/Foundation/Platform/PoseidonInit.hpp>
#include <Poseidon/Graphics/Core/Engine.hpp>
#include <Poseidon/Graphics/Dummy/GraphicsEngineDummy.hpp>

using Poseidon::CreateEngineDummy;
using Poseidon::GEngine;
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/Streams/FileInfo.h>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include <fstream>
#include <algorithm>
#ifndef _WIN32
#include <sys/stat.h>
#include <strings.h>

#endif

class TcPboAppFrameFunctions : public Poseidon::Foundation::AppFrameFunctions
{
  public:
    void ErrorMessage(Poseidon::Foundation::ErrorMessageLevel, const char*, va_list) override {}
    void ErrorMessage(const char*, va_list) override {}
    void WarningMessage(const char*, va_list) override {}
    void ShowMessage(int, const char*, va_list) override {}
    DWORD TickCount() override { return GetTickCount(); }
};

static TcPboAppFrameFunctions g_appFrame;
static bool g_engineInitialized = false;

static void EnsureEngineInit()
{
    if (g_engineInitialized)
        return;
    Poseidon::Foundation::CurrentAppFrameFunctions = &g_appFrame;
    SetMemorySystemReady(true);
    InitLibraryElement();
    Poseidon::InitDefaults();
    GEngine = CreateEngineDummy();
    g_engineInitialized = true;
}

struct PboEntry
{
    std::string name;
    long length; // stored size
    long time;   // unix timestamp
    int compressedMagic;
    int uncompressedSize;
};

static void CollectEntry(const FileInfoO& fi, const FileBankType*, void* ctx)
{
    auto* entries = static_cast<std::vector<PboEntry>*>(ctx);
    PboEntry e;
    e.name = (const char*)fi.name;
    e.length = fi.length;
    e.time = fi.time;
    e.compressedMagic = fi.compressedMagic;
    e.uncompressedSize = fi.uncompressedSize;
    entries->push_back(e);
}

struct PboArchive
{
    QFBank bank;
    std::vector<PboEntry> entries;
    int currentIndex;
    int openMode;
    std::string archiveName;
    tProcessDataProc processDataProc;
    tChangeVolProc changeVolProc;
};

// QFBank::open() appends the .pbo extension itself.
static std::string StripPboExtension(const std::string& path)
{
    if (path.size() >= 4)
    {
        std::string ext = path.substr(path.size() - 4);
        std::string lower;
        lower.reserve(ext.size());
        for (char c : ext)
            lower += static_cast<char>(tolower(c));
        if (lower == ".pbo")
            return path.substr(0, path.size() - 4);
    }
    return path;
}

static int UnixTimeToDos(long unixTime)
{
    if (unixTime <= 0)
        return 0;
    time_t t_val = static_cast<time_t>(unixTime);
    struct tm* t = localtime(&t_val);
    if (!t)
        return 0;
    int year = t->tm_year + 1900;
    if (year < 1980)
        year = 1980;
    return ((year - 1980) << 25) | ((t->tm_mon + 1) << 21) | (t->tm_mday << 16) | (t->tm_hour << 11) |
           (t->tm_min << 5) | (t->tm_sec / 2);
}

static void NormalizePathSeparators(std::string& path)
{
#ifdef _WIN32
    for (char& c : path)
        if (c == '/')
            c = '\\';
#else
    for (char& c : path)
        if (c == '\\')
            c = '/';
#endif
}

static char PathSeparator()
{
#ifdef _WIN32
    return '\\';
#else
    return '/';
#endif
}

static bool CreateParentDirs(const std::string& filePath)
{
    for (size_t i = 0; i < filePath.size(); i++)
    {
        if (filePath[i] == '\\' || filePath[i] == '/')
        {
            std::string dir = filePath.substr(0, i);
            if (!dir.empty())
            {
#ifdef _WIN32
                CreateDirectoryA(dir.c_str(), nullptr);
#else
                mkdir(dir.c_str(), 0755);
#endif
            }
        }
    }
    return true;
}

#ifndef _WIN32
static bool WriteFileData(const std::string& path, const void* data, size_t size)
{
    FILE* f = fopen(path.c_str(), "wb");
    if (!f)
        return false;
    if (size > 0)
    {
        if (fwrite(data, 1, size, f) != size)
        {
            fclose(f);
            return false;
        }
    }
    fclose(f);
    return true;
}
#endif

extern "C"
{
    WCX_HANDLE __stdcall OpenArchive(tOpenArchiveData* ArchiveData)
    {
        EnsureEngineInit();

        auto* arc = new PboArchive();
        arc->archiveName = ArchiveData->ArcName;
        arc->currentIndex = 0;
        arc->openMode = ArchiveData->OpenMode;
        arc->processDataProc = nullptr;
        arc->changeVolProc = nullptr;

        std::string bankName = StripPboExtension(arc->archiveName);
        if (!arc->bank.open(RString(bankName.c_str())))
        {
            ArchiveData->OpenResult = E_EOPEN;
            delete arc;
            return (WCX_HANDLE)0;
        }
        arc->bank.Lock();

        if (arc->bank.error())
        {
            ArchiveData->OpenResult = E_BAD_ARCHIVE;
            arc->bank.Unlock();
            delete arc;
            return (WCX_HANDLE)0;
        }

        arc->bank.ForEach(CollectEntry, &arc->entries);
        ArchiveData->OpenResult = 0;
        return (WCX_HANDLE)arc;
    }

    int __stdcall ReadHeader(WCX_HANDLE hArcData, tHeaderData* HeaderData)
    {
        auto* arc = reinterpret_cast<PboArchive*>(hArcData);
        if (!arc || arc->currentIndex >= static_cast<int>(arc->entries.size()))
            return E_END_ARCHIVE;

        const auto& e = arc->entries[arc->currentIndex];
        memset(HeaderData, 0, sizeof(tHeaderData));
        strncpy(HeaderData->ArcName, arc->archiveName.c_str(), sizeof(HeaderData->ArcName) - 1);

        std::string name = e.name;
        NormalizePathSeparators(name);
        strncpy(HeaderData->FileName, name.c_str(), sizeof(HeaderData->FileName) - 1);

        bool compressed = (e.compressedMagic == CompMagic);
        HeaderData->PackSize = e.length;
        HeaderData->UnpSize = compressed ? e.uncompressedSize : e.length;
        HeaderData->FileTime = UnixTimeToDos(e.time);
        HeaderData->FileAttr = FILE_ATTRIBUTE_NORMAL;

        return 0;
    }

    int __stdcall ReadHeaderEx(WCX_HANDLE hArcData, tHeaderDataEx* HeaderData)
    {
        auto* arc = reinterpret_cast<PboArchive*>(hArcData);
        if (!arc || arc->currentIndex >= static_cast<int>(arc->entries.size()))
            return E_END_ARCHIVE;

        const auto& e = arc->entries[arc->currentIndex];
        memset(HeaderData, 0, sizeof(tHeaderDataEx));
        strncpy(HeaderData->ArcName, arc->archiveName.c_str(), sizeof(HeaderData->ArcName) - 1);

        std::string name = e.name;
        NormalizePathSeparators(name);
        strncpy(HeaderData->FileName, name.c_str(), sizeof(HeaderData->FileName) - 1);

        bool compressed = (e.compressedMagic == CompMagic);
        HeaderData->PackSize = static_cast<unsigned int>(e.length);
        HeaderData->PackSizeHigh = 0;
        HeaderData->UnpSize =
            compressed ? static_cast<unsigned int>(e.uncompressedSize) : static_cast<unsigned int>(e.length);
        HeaderData->UnpSizeHigh = 0;
        HeaderData->FileTime = UnixTimeToDos(e.time);
        HeaderData->FileAttr = FILE_ATTRIBUTE_NORMAL;

        return 0;
    }

    int __stdcall ProcessFile(WCX_HANDLE hArcData, int Operation, char* DestPath, char* DestName)
    {
        auto* arc = reinterpret_cast<PboArchive*>(hArcData);
        if (!arc || arc->currentIndex >= static_cast<int>(arc->entries.size()))
            return E_END_ARCHIVE;

        const auto& e = arc->entries[arc->currentIndex];
        arc->currentIndex++;

        if (Operation == PK_SKIP || Operation == PK_TEST)
            return 0;

        if (Operation == PK_EXTRACT)
        {
            std::string outPath;
            if (DestPath && DestPath[0])
            {
                outPath = DestPath;
                if (outPath.back() != '\\' && outPath.back() != '/')
                    outPath += PathSeparator();
            }
            if (DestName && DestName[0])
                outPath += DestName;
            else
            {
                std::string name = e.name;
                NormalizePathSeparators(name);
                outPath += name;
            }

            CreateParentDirs(outPath);

            Ref<IFileBuffer> data = arc->bank.Read(e.name.c_str());
            if (!data)
                return E_EREAD;

#ifdef _WIN32
            HANDLE hFile =
                CreateFileA(outPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE)
                return E_ECREATE;

            if (data->GetSize() > 0)
            {
                DWORD written = 0;
                if (!WriteFile(hFile, data->GetData(), static_cast<DWORD>(data->GetSize()), &written, nullptr))
                {
                    CloseHandle(hFile);
                    return E_EWRITE;
                }
            }
            CloseHandle(hFile);
#else
            if (!WriteFileData(outPath, data->GetData(), data->GetSize()))
                return E_ECREATE;
#endif

            if (arc->processDataProc)
                arc->processDataProc(const_cast<char*>(outPath.c_str()), static_cast<int>(data->GetSize()));
        }

        return 0;
    }

    int __stdcall CloseArchive(WCX_HANDLE hArcData)
    {
        auto* arc = reinterpret_cast<PboArchive*>(hArcData);
        if (!arc)
            return E_ECLOSE;
        arc->bank.Unlock();
        delete arc;
        return 0;
    }

    void __stdcall SetChangeVolProc(WCX_HANDLE hArcData, tChangeVolProc pChangeVolProc)
    {
        auto* arc = reinterpret_cast<PboArchive*>(hArcData);
        if (arc)
            arc->changeVolProc = pChangeVolProc;
    }

    void __stdcall SetProcessDataProc(WCX_HANDLE hArcData, tProcessDataProc pProcessDataProc)
    {
        auto* arc = reinterpret_cast<PboArchive*>(hArcData);
        if (arc)
            arc->processDataProc = pProcessDataProc;
    }

    int __stdcall GetPackerCaps()
    {
        return PK_CAPS_MULTIPLE | PK_CAPS_BY_CONTENT | PK_CAPS_SEARCHTEXT;
    }

    BOOL __stdcall CanYouHandleThisFile(char* FileName)
    {
        if (!FileName)
            return FALSE;
        size_t len = strlen(FileName);
        if (len < 4)
            return FALSE;
        const char* ext = FileName + len - 4;
#ifdef _WIN32
        return (_stricmp(ext, ".pbo") == 0) ? TRUE : FALSE;
#else
        return (strcasecmp(ext, ".pbo") == 0) ? TRUE : FALSE;
#endif
    }

} // extern "C"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        EnsureEngineInit();
    return TRUE;
}
#else
__attribute__((constructor)) static void PluginInit()
{
    EnsureEngineInit();
}
#endif
