// PBO regression test -- verifies ALL game PBOs can be opened, enumerated,
// and read (including compressed entries).

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../Support/test_fixtures.hpp"
#include <filesystem>
#include <algorithm>
#include <ctype.h>
#include <stdio.h>
#include <catch2/catch_message.hpp>
#include <format>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct PboScanEntry
{
    std::string name;
    int length;
    int uncompressedSize;
    int compressedMagic;
};

static void CollectEntry(const FileInfoO& fi, const FileBankType*, void* ctx)
{
    auto* vec = static_cast<std::vector<PboScanEntry>*>(ctx);
    PboScanEntry e;
    e.name = (const char*)fi.name;
    e.length = fi.length;
    e.uncompressedSize = fi.uncompressedSize;
    e.compressedMagic = fi.compressedMagic;
    vec->push_back(e);
}

static std::string GetGameDir(const char* subdir)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%s", GAME_DATA_DIR, subdir);
    return path;
}

static std::string StripPboExt(const std::string& path)
{
    if (path.size() >= 4)
    {
        auto tail = path.substr(path.size() - 4);
        std::string lower = tail;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == ".pbo")
            return path.substr(0, path.size() - 4);
    }
    return path;
}

TEST_CASE("PBO Game Scan: All AddOns PBOs open and enumerate", "[qstream][pbo][gamescan]")
{
    std::string addonsDir = GetGameDir("AddOns");
    if (!fs::exists(addonsDir))
    {
        SKIP("Game AddOns directory not available");
        return;
    }

    int pboCount = 0, totalEntries = 0, failedPbos = 0;

    for (const auto& entry : fs::directory_iterator(addonsDir))
    {
        if (!entry.is_regular_file())
            continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".pbo")
            continue;

        pboCount++;
        QFBank bank;
        bool opened = bank.open(RString(StripPboExt(entry.path().string()).c_str()));
        if (!opened)
        {
            failedPbos++;
            WARN("Failed: " << entry.path().filename().string());
            continue;
        }
        bank.Lock();
        if (bank.error())
        {
            failedPbos++;
            continue;
        }

        std::vector<PboScanEntry> entries;
        bank.ForEach(CollectEntry, &entries);
        REQUIRE(entries.size() > 0);
        totalEntries += (int)entries.size();

        for (const auto& e : entries)
        {
            REQUIRE(!e.name.empty());
            REQUIRE(e.length >= 0);
        }
        bank.Unlock();
    }
    REQUIRE(pboCount > 0);
    REQUIRE(failedPbos == 0);
}

TEST_CASE("PBO Game Scan: All AddOns PBOs -- read every file", "[qstream][pbo][gamescan]")
{
    std::string addonsDir = GetGameDir("AddOns");
    if (!fs::exists(addonsDir))
    {
        SKIP("Game AddOns not available");
        return;
    }

    int filesRead = 0, readErrors = 0;

    for (const auto& entry : fs::directory_iterator(addonsDir))
    {
        if (!entry.is_regular_file())
            continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".pbo")
            continue;

        QFBank bank;
        if (!bank.open(RString(StripPboExt(entry.path().string()).c_str())))
            continue;
        bank.Lock();
        if (bank.error())
            continue;

        std::vector<PboScanEntry> entries;
        bank.ForEach(CollectEntry, &entries);

        for (const auto& e : entries)
        {
            if (e.length == 0 && e.uncompressedSize == 0)
                continue;
            Ref<IFileBuffer> data = bank.Read(e.name.c_str());
            if (!data || data->GetSize() == 0)
            {
                readErrors++;
                WARN("Read failed: " << entry.path().filename().string() << "/" << e.name);
                continue;
            }
            if (e.compressedMagic != 0 && e.uncompressedSize > 0)
                CHECK(data->GetSize() == e.uncompressedSize);
            else if (e.compressedMagic == 0)
                CHECK(data->GetSize() == e.length);
            filesRead++;
        }
        bank.Unlock();
    }
    REQUIRE(filesRead > 0);
    REQUIRE(readErrors == 0);
}

TEST_CASE("PBO Game Scan: DTA PBOs open and read", "[qstream][pbo][gamescan]")
{
    std::string dtaDir = GetGameDir("DTA");
    if (!fs::exists(dtaDir))
    {
        SKIP("Game DTA not available");
        return;
    }

    int pboCount = 0, totalEntries = 0, readErrors = 0;

    for (const auto& entry : fs::directory_iterator(dtaDir))
    {
        if (!entry.is_regular_file())
            continue;
        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".pbo")
            continue;

        pboCount++;
        QFBank bank;
        REQUIRE(bank.open(RString(StripPboExt(entry.path().string()).c_str())));
        bank.Lock();
        REQUIRE(!bank.error());

        std::vector<PboScanEntry> entries;
        bank.ForEach(CollectEntry, &entries);
        REQUIRE(entries.size() > 0);
        totalEntries += (int)entries.size();

        for (const auto& e : entries)
        {
            Ref<IFileBuffer> data = bank.Read(e.name.c_str());
            // Zero-length entries are PBO metadata/property markers -- skip them
            if (e.length == 0 && e.uncompressedSize == 0)
                continue;
            if (!data || data->GetSize() == 0)
            {
                readErrors++;
                WARN("Read failed: " << entry.path().filename().string() << "/" << e.name);
            }
        }
        bank.Unlock();
    }
    REQUIRE(pboCount > 0);
    REQUIRE(readErrors == 0);
}

TEST_CASE("PBO Game Scan: Compressed entries decompress correctly", "[qstream][pbo][gamescan]")
{
    std::string addonsDir = GetGameDir("AddOns");
    if (!fs::exists(addonsDir))
    {
        SKIP("Game AddOns not available");
        return;
    }

    int compressedCount = 0, decompressOk = 0;

    for (const auto& pboEntry : fs::directory_iterator(addonsDir))
    {
        if (!pboEntry.is_regular_file())
            continue;
        auto ext = pboEntry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".pbo")
            continue;

        QFBank bank;
        if (!bank.open(RString(StripPboExt(pboEntry.path().string()).c_str())))
            continue;
        bank.Lock();
        if (bank.error())
            continue;

        std::vector<PboScanEntry> entries;
        bank.ForEach(CollectEntry, &entries);

        for (const auto& e : entries)
        {
            if (e.compressedMagic == 0)
                continue;
            compressedCount++;
            Ref<IFileBuffer> data = bank.Read(e.name.c_str());
            REQUIRE(data != nullptr);
            REQUIRE(data->GetSize() > 0);
            REQUIRE(data->GetData() != nullptr);
            CHECK(data->GetSize() == e.uncompressedSize);
            decompressOk++;
        }
        bank.Unlock();
    }
    if (compressedCount > 0)
        REQUIRE(decompressOk == compressedCount);
    else
        WARN("No compressed entries found");
}
