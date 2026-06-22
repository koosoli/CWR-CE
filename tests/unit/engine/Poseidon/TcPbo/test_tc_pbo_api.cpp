#include <catch2/catch_test_macros.hpp>
#include "../test_fixtures.hpp"

#ifdef _WIN32
#include <windows.h>

// WCX SDK types (matching wcxhead.h)
#define E_END_ARCHIVE 10
#define E_EOPEN 15
#define PK_OM_LIST 0
#define PK_OM_EXTRACT 1
#define PK_SKIP 0
#define PK_EXTRACT 2
#define PK_CAPS_MULTIPLE 4
#define PK_CAPS_BY_CONTENT 64
#define PK_CAPS_SEARCHTEXT 128

typedef struct
{
    char ArcName[260];
    char FileName[260];
    int Flags;
    int PackSize;
    int UnpSize;
    int HostOS;
    int FileCRC;
    int FileTime;
    int UnpVer;
    int Method;
    int FileAttr;
    char* CmtBuf;
    int CmtBufSize;
    int CmtSize;
    int CmtState;
} tHeaderData;

typedef struct
{
    char ArcName[1024];
    char FileName[1024];
    int Flags;
    unsigned int PackSize;
    unsigned int PackSizeHigh;
    unsigned int UnpSize;
    unsigned int UnpSizeHigh;
    int HostOS;
    int FileCRC;
    int FileTime;
    int UnpVer;
    int Method;
    int FileAttr;
    char* CmtBuf;
    int CmtBufSize;
    int CmtSize;
    int CmtState;
    char Reserved[1024];
} tHeaderDataEx;

typedef struct
{
    char* ArcName;
    int OpenMode;
    int OpenResult;
    char* CmtBuf;
    int CmtBufSize;
    int CmtSize;
    int CmtState;
} tOpenArchiveData;

typedef int(__stdcall* tChangeVolProc)(char* ArcName, int Mode);
typedef int(__stdcall* tProcessDataProc)(char* FileName, int Size);

typedef HANDLE(__stdcall* fnOpenArchive)(tOpenArchiveData*);
typedef int(__stdcall* fnReadHeader)(HANDLE, tHeaderData*);
typedef int(__stdcall* fnReadHeaderEx)(HANDLE, tHeaderDataEx*);
typedef int(__stdcall* fnProcessFile)(HANDLE, int, char*, char*);
typedef int(__stdcall* fnCloseArchive)(HANDLE);
typedef void(__stdcall* fnSetChangeVolProc)(HANDLE, tChangeVolProc);
typedef void(__stdcall* fnSetProcessDataProc)(HANDLE, tProcessDataProc);
typedef int(__stdcall* fnGetPackerCaps)();
typedef BOOL(__stdcall* fnCanYouHandleThisFile)(char*);

struct WcxPlugin
{
    HMODULE hModule = nullptr;
    fnOpenArchive OpenArchive = nullptr;
    fnReadHeader ReadHeader = nullptr;
    fnReadHeaderEx ReadHeaderEx = nullptr;
    fnProcessFile ProcessFile = nullptr;
    fnCloseArchive CloseArchive = nullptr;
    fnSetChangeVolProc SetChangeVolProc = nullptr;
    fnSetProcessDataProc SetProcessDataProc = nullptr;
    fnGetPackerCaps GetPackerCaps = nullptr;
    fnCanYouHandleThisFile CanYouHandleThisFile = nullptr;

    bool Load(const char* dllPath)
    {
        hModule = LoadLibraryA(dllPath);
        if (!hModule)
            return false;
        OpenArchive = (fnOpenArchive)GetProcAddress(hModule, "OpenArchive");
        ReadHeader = (fnReadHeader)GetProcAddress(hModule, "ReadHeader");
        ReadHeaderEx = (fnReadHeaderEx)GetProcAddress(hModule, "ReadHeaderEx");
        ProcessFile = (fnProcessFile)GetProcAddress(hModule, "ProcessFile");
        CloseArchive = (fnCloseArchive)GetProcAddress(hModule, "CloseArchive");
        SetChangeVolProc = (fnSetChangeVolProc)GetProcAddress(hModule, "SetChangeVolProc");
        SetProcessDataProc = (fnSetProcessDataProc)GetProcAddress(hModule, "SetProcessDataProc");
        GetPackerCaps = (fnGetPackerCaps)GetProcAddress(hModule, "GetPackerCaps");
        CanYouHandleThisFile = (fnCanYouHandleThisFile)GetProcAddress(hModule, "CanYouHandleThisFile");
        return true;
    }

    ~WcxPlugin()
    {
        if (hModule)
            FreeLibrary(hModule);
    }
};

static std::string GetPluginPath()
{
    char path[MAX_PATH];
#ifdef _WIN64
    snprintf(path, sizeof(path), "%s\\pbo.wcx64", TestFixtures::GetExecutableDirectory());
#else
    snprintf(path, sizeof(path), "%s\\pbo.wcx", TestFixtures::GetExecutableDirectory());
#endif
    return path;
}

static std::string GetPboFixturePath(const char* name)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s\\fixtures\\pbo\\%s", TestFixtures::GetExecutableDirectory(), name);
    return path;
}

TEST_CASE("TcPbo - Plugin DLL loads", "[tcpbo]")
{
    WcxPlugin wcx;
    std::string dllPath = GetPluginPath();
    INFO("DLL path: " << dllPath);
    REQUIRE(wcx.Load(dllPath.c_str()));
}

TEST_CASE("TcPbo - All required exports present", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    REQUIRE(wcx.OpenArchive != nullptr);
    REQUIRE(wcx.ReadHeader != nullptr);
    REQUIRE(wcx.ReadHeaderEx != nullptr);
    REQUIRE(wcx.ProcessFile != nullptr);
    REQUIRE(wcx.CloseArchive != nullptr);
    REQUIRE(wcx.SetChangeVolProc != nullptr);
    REQUIRE(wcx.SetProcessDataProc != nullptr);
    REQUIRE(wcx.GetPackerCaps != nullptr);
    REQUIRE(wcx.CanYouHandleThisFile != nullptr);
}

TEST_CASE("TcPbo - GetPackerCaps returns read-only multi-file caps", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));
    int caps = wcx.GetPackerCaps();
    REQUIRE((caps & PK_CAPS_MULTIPLE) != 0);
    REQUIRE((caps & PK_CAPS_BY_CONTENT) != 0);
    REQUIRE((caps & PK_CAPS_SEARCHTEXT) != 0);
}

TEST_CASE("TcPbo - CanYouHandleThisFile", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    REQUIRE(wcx.CanYouHandleThisFile(const_cast<char*>("data.pbo")) == TRUE);
    REQUIRE(wcx.CanYouHandleThisFile(const_cast<char*>("archive.PBO")) == TRUE);
    REQUIRE(wcx.CanYouHandleThisFile(const_cast<char*>("C:\\game\\data.pbo")) == TRUE);
    REQUIRE(wcx.CanYouHandleThisFile(const_cast<char*>("readme.txt")) == FALSE);
    REQUIRE(wcx.CanYouHandleThisFile(const_cast<char*>("short")) == FALSE);
}

TEST_CASE("TcPbo - OpenArchive and list addon_fixture.pbo", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    std::string pboPath = GetPboFixturePath("addon_fixture.pbo");
    tOpenArchiveData archData = {};
    archData.ArcName = const_cast<char*>(pboPath.c_str());
    archData.OpenMode = PK_OM_LIST;

    HANDLE h = wcx.OpenArchive(&archData);
    REQUIRE(h != nullptr);
    REQUIRE(archData.OpenResult == 0);

    std::vector<std::string> fileNames;
    tHeaderDataEx header;
    while (wcx.ReadHeaderEx(h, &header) == 0)
    {
        fileNames.push_back(header.FileName);
        REQUIRE(header.UnpSize > 0);
        REQUIRE(header.FileTime != 0);
        int rc = wcx.ProcessFile(h, PK_SKIP, nullptr, nullptr);
        REQUIRE(rc == 0);
    }

    REQUIRE(fileNames.size() > 0);
    REQUIRE(wcx.CloseArchive(h) == 0);
}

TEST_CASE("TcPbo - ReadHeader legacy compat", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    std::string pboPath = GetPboFixturePath("addon_fixture.pbo");
    tOpenArchiveData archData = {};
    archData.ArcName = const_cast<char*>(pboPath.c_str());
    archData.OpenMode = PK_OM_LIST;

    HANDLE h = wcx.OpenArchive(&archData);
    REQUIRE(h != nullptr);

    tHeaderData header;
    int rc = wcx.ReadHeader(h, &header);
    REQUIRE(rc == 0);
    REQUIRE(strlen(header.FileName) > 0);
    REQUIRE(header.UnpSize > 0);

    wcx.ProcessFile(h, PK_SKIP, nullptr, nullptr);
    wcx.CloseArchive(h);
}

TEST_CASE("TcPbo - Extract file from addon_fixture.pbo", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    std::string pboPath = GetPboFixturePath("addon_fixture.pbo");
    tOpenArchiveData archData = {};
    archData.ArcName = const_cast<char*>(pboPath.c_str());
    archData.OpenMode = PK_OM_EXTRACT;

    HANDLE h = wcx.OpenArchive(&archData);
    REQUIRE(h != nullptr);

    tHeaderDataEx header;
    REQUIRE(wcx.ReadHeaderEx(h, &header) == 0);

    char tempDir[MAX_PATH];
    snprintf(tempDir, sizeof(tempDir), "%s\\tcpbo_extract_test", TestFixtures::GetExecutableDirectory());
    CreateDirectoryA(tempDir, nullptr);

    int rc = wcx.ProcessFile(h, PK_EXTRACT, tempDir, nullptr);
    REQUIRE(rc == 0);

    char extractedPath[MAX_PATH];
    snprintf(extractedPath, sizeof(extractedPath), "%s\\%s", tempDir, header.FileName);
    DWORD attrs = GetFileAttributesA(extractedPath);
    REQUIRE(attrs != INVALID_FILE_ATTRIBUTES);

    HANDLE hFile = CreateFileA(extractedPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    REQUIRE(hFile != INVALID_HANDLE_VALUE);
    DWORD fileSize = GetFileSize(hFile, nullptr);
    REQUIRE(fileSize > 0);
    REQUIRE(fileSize == header.UnpSize);
    CloseHandle(hFile);

    DeleteFileA(extractedPath);
    RemoveDirectoryA(tempDir);

    wcx.CloseArchive(h);
}

TEST_CASE("TcPbo - Open mission_fixture.Intro.pbo (compressed entries)", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    std::string pboPath = GetPboFixturePath("mission_fixture.Intro.pbo");
    tOpenArchiveData archData = {};
    archData.ArcName = const_cast<char*>(pboPath.c_str());
    archData.OpenMode = PK_OM_LIST;

    HANDLE h = wcx.OpenArchive(&archData);
    REQUIRE(h != nullptr);
    REQUIRE(archData.OpenResult == 0);

    int entryCount = 0;
    tHeaderDataEx header;
    while (wcx.ReadHeaderEx(h, &header) == 0)
    {
        entryCount++;
        wcx.ProcessFile(h, PK_SKIP, nullptr, nullptr);
    }
    REQUIRE(entryCount > 0);

    wcx.CloseArchive(h);
}

TEST_CASE("TcPbo - OpenArchive fails for non-existent file", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    tOpenArchiveData archData = {};
    char fakePath[] = "C:\\nonexistent\\fake.pbo";
    archData.ArcName = fakePath;
    archData.OpenMode = PK_OM_LIST;

    HANDLE h = wcx.OpenArchive(&archData);
    REQUIRE(h == nullptr);
    REQUIRE(archData.OpenResult != 0);
}

TEST_CASE("TcPbo - SetChangeVolProc and SetProcessDataProc accept callbacks", "[tcpbo]")
{
    WcxPlugin wcx;
    REQUIRE(wcx.Load(GetPluginPath().c_str()));

    std::string pboPath = GetPboFixturePath("addon_fixture.pbo");
    tOpenArchiveData archData = {};
    archData.ArcName = const_cast<char*>(pboPath.c_str());
    archData.OpenMode = PK_OM_LIST;

    HANDLE h = wcx.OpenArchive(&archData);
    REQUIRE(h != nullptr);

    // Should not crash when setting callbacks (can be nullptr or valid)
    wcx.SetChangeVolProc(h, nullptr);
    wcx.SetProcessDataProc(h, nullptr);

    wcx.CloseArchive(h);
}

#else
// Non-Windows: no tests (plugin is Windows-only)
TEST_CASE("TcPbo - Plugin is Windows-only", "[tcpbo]")
{
    SUCCEED("WCX plugin tests skipped on non-Windows platforms");
}
#endif
