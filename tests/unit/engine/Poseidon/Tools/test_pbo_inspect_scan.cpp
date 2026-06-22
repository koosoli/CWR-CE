// PBO+Inspect regression test -- extracts P3D/FXY from game PBOs and runs
// InspectModel/InspectFont, exercising the same code path as Studio's UsedBy scan.

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <ctype.h>
#include <stdio.h>
#include <catch2/catch_message.hpp>
#include <exception>
#include <format>
#include <string>
#include <system_error>
#include <vector>
using namespace Poseidon;
#include "../test_fixtures.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <atomic>

namespace fs = std::filesystem;
using namespace Poseidon;

struct ScanEntry
{
    std::string name;
    int length;
    int uncompressedSize;
    int compressedMagic;
};

static void CollectScanEntry(const FileInfoO& fi, const FileBankType*, void* ctx)
{
    auto* vec = static_cast<std::vector<ScanEntry>*>(ctx);
    ScanEntry e;
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

static bool EndsWithCI(const std::string& s, const std::string& suffix)
{
    if (s.size() < suffix.size())
        return false;
    std::string tail = s.substr(s.size() - suffix.size());
    std::transform(tail.begin(), tail.end(), tail.begin(), ::tolower);
    return tail == suffix;
}

static std::atomic<int> gTmpId{0};

TEST_CASE("PBO+Inspect: InspectModel on all P3D in AddOns PBOs", "[tools][pbo][gamescan][inspect]")
{
    std::string addonsDir = GetGameDir("AddOns");
    if (!fs::exists(addonsDir))
    {
        SKIP("Game AddOns not available");
        return;
    }

    int modelsInspected = 0, modelsValid = 0, inspectErrors = 0;
    auto tempDir = fs::temp_directory_path();

    for (const auto& pboEntry : fs::directory_iterator(addonsDir))
    {
        if (!pboEntry.is_regular_file())
            continue;
        if (!EndsWithCI(pboEntry.path().string(), ".pbo"))
            continue;

        QFBank bank;
        if (!bank.open(RString(StripPboExt(pboEntry.path().string()).c_str())))
            continue;
        bank.Lock();
        if (bank.error())
            continue;

        std::vector<ScanEntry> entries;
        bank.ForEach(CollectScanEntry, &entries);

        for (const auto& e : entries)
        {
            if (!EndsWithCI(e.name, ".p3d"))
                continue;

            Ref<IFileBuffer> data = bank.Read(e.name.c_str());
            if (!data || data->GetSize() == 0)
                continue;

            int id = gTmpId++;
            std::string tmpPath = (tempDir / ("inspect_p3d_" + std::to_string(id) + ".p3d")).string();
            {
                std::ofstream out(tmpPath, std::ios::binary);
                REQUIRE(out.good());
                out.write(static_cast<const char*>(data->GetData()), data->GetSize());
            }

            try
            {
                auto info = InspectModel(tmpPath);
                modelsInspected++;
                if (info.valid)
                    modelsValid++;
            }
            catch (const std::exception& ex)
            {
                inspectErrors++;
                WARN("InspectModel error: " << pboEntry.path().filename().string() << "/" << e.name << ": "
                                            << ex.what());
            }
            catch (...)
            {
                inspectErrors++;
                WARN("InspectModel crash: " << pboEntry.path().filename().string() << "/" << e.name);
            }

            std::error_code ec;
            fs::remove(tmpPath, ec);
        }
        bank.Unlock();
    }
    REQUIRE(modelsInspected > 0);
    REQUIRE(inspectErrors == 0);
}

TEST_CASE("PBO+Inspect: InspectFont on all FXY in DTA PBOs", "[tools][pbo][gamescan][inspect]")
{
    std::string dtaDir = GetGameDir("DTA");
    if (!fs::exists(dtaDir))
    {
        SKIP("Game DTA not available");
        return;
    }

    int fontsInspected = 0, fontsValid = 0, inspectErrors = 0;
    auto tempDir = fs::temp_directory_path();

    for (const auto& pboEntry : fs::directory_iterator(dtaDir))
    {
        if (!pboEntry.is_regular_file())
            continue;
        if (!EndsWithCI(pboEntry.path().string(), ".pbo"))
            continue;

        QFBank bank;
        if (!bank.open(RString(StripPboExt(pboEntry.path().string()).c_str())))
            continue;
        bank.Lock();
        if (bank.error())
            continue;

        std::vector<ScanEntry> entries;
        bank.ForEach(CollectScanEntry, &entries);

        for (const auto& e : entries)
        {
            if (!EndsWithCI(e.name, ".fxy"))
                continue;

            Ref<IFileBuffer> data = bank.Read(e.name.c_str());
            if (!data || data->GetSize() == 0)
                continue;

            int id = gTmpId++;
            std::string tmpPath = (tempDir / ("inspect_fxy_" + std::to_string(id) + ".fxy")).string();
            {
                std::ofstream out(tmpPath, std::ios::binary);
                REQUIRE(out.good());
                out.write(static_cast<const char*>(data->GetData()), data->GetSize());
            }

            try
            {
                auto info = InspectFont(tmpPath);
                fontsInspected++;
                if (info.valid)
                    fontsValid++;
            }
            catch (const std::exception& ex)
            {
                inspectErrors++;
                WARN("InspectFont error: " << pboEntry.path().filename().string() << "/" << e.name << ": "
                                           << ex.what());
            }
            catch (...)
            {
                inspectErrors++;
                WARN("InspectFont crash: " << pboEntry.path().filename().string() << "/" << e.name);
            }

            std::error_code ec;
            fs::remove(tmpPath, ec);
        }
        bank.Unlock();
    }
    if (fontsInspected > 0)
    {
        REQUIRE(inspectErrors == 0);
    }
    else
    {
        WARN("No FXY files found in DTA PBOs");
    }
}

TEST_CASE("PBO+Inspect: InspectModel on DTA Data3D P3D files", "[tools][pbo][gamescan][inspect]")
{
    std::string dtaDir = GetGameDir("DTA");
    if (!fs::exists(dtaDir))
    {
        SKIP("Game DTA not available");
        return;
    }

    // Data3D.pbo typically contains the main game models
    std::string data3dPbo = dtaDir + "/Data3D";
    QFBank bank;
    if (!bank.open(RString(data3dPbo.c_str())))
    {
        SKIP("Data3D.pbo not found");
        return;
    }
    bank.Lock();
    if (bank.error())
    {
        SKIP("Data3D.pbo lock error");
        return;
    }

    std::vector<ScanEntry> entries;
    bank.ForEach(CollectScanEntry, &entries);

    int modelsInspected = 0, inspectErrors = 0;
    auto tempDir = fs::temp_directory_path();

    for (const auto& e : entries)
    {
        if (!EndsWithCI(e.name, ".p3d"))
            continue;

        Ref<IFileBuffer> data = bank.Read(e.name.c_str());
        if (!data || data->GetSize() == 0)
            continue;

        int id = gTmpId++;
        std::string tmpPath = (tempDir / ("inspect_data3d_" + std::to_string(id) + ".p3d")).string();
        {
            std::ofstream out(tmpPath, std::ios::binary);
            REQUIRE(out.good());
            out.write(static_cast<const char*>(data->GetData()), data->GetSize());
        }

        try
        {
            auto info = InspectModel(tmpPath);
            modelsInspected++;
            if (!info.valid)
                WARN("Invalid P3D in Data3D: " << e.name << " (" << info.errorMessage << ")");
        }
        catch (const std::exception& ex)
        {
            inspectErrors++;
            WARN("InspectModel error on Data3D/" << e.name << ": " << ex.what());
        }
        catch (...)
        {
            inspectErrors++;
            WARN("InspectModel crash on Data3D/" << e.name);
        }

        std::error_code ec;
        fs::remove(tmpPath, ec);
    }
    bank.Unlock();

    if (modelsInspected > 0)
        REQUIRE(inspectErrors == 0);
    else
        WARN("No P3D files found in Data3D.pbo");
}
