#include <catch2/catch_test_macros.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/IO/PackFiles.hpp>
#include "../Support/test_fixtures.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <format>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

using namespace TestFixtures;
namespace fs = std::filesystem;

// Collect file entries from ForEach callback
struct PboEntry
{
    std::string name;
    long length;
    long time;
    int compressedMagic;
    int uncompressedSize;
};

struct PboCollect
{
    std::vector<PboEntry>* entries;
};

static void CollectPboEntry(const FileInfoO& fi, const FileBankType*, void* ctx)
{
    auto* c = static_cast<PboCollect*>(ctx);
    PboEntry e;
    e.name = (const char*)fi.name;
    e.length = fi.length;
    e.time = fi.time;
    e.compressedMagic = fi.compressedMagic;
    e.uncompressedSize = fi.uncompressedSize;
    c->entries->push_back(e);
}

static std::string ReadFirstPboEntryName(const fs::path& pbo)
{
    std::ifstream in(pbo, std::ios::binary);
    REQUIRE(in.good());

    std::string name;
    char c = 0;
    while (in.get(c))
    {
        if (c == '\0')
        {
            return name;
        }
        name.push_back(c);
    }
    return name;
}

TEST_CASE("QFBank - Open PBO fixture (mission_fixture.Intro)", "[qstream][pbo][roundtrip]")
{
    REQUIRE_FIXTURE("pbo/mission_fixture.Intro.pbo");

    // Build bank name by stripping .pbo (QFBank::open appends it)
    std::string fixture = GetTestFixturePath("pbo/mission_fixture.Intro.pbo");
    std::string bankName = fixture.substr(0, fixture.size() - 4);

    QFBank bank;
    bool opened = bank.open(RString(bankName.c_str()));
    REQUIRE(opened);

    bank.Lock();
    REQUIRE(bank.error() == false);

    std::vector<PboEntry> entries;
    PboCollect ctx{&entries};
    bank.ForEach(CollectPboEntry, &ctx);

    REQUIRE(entries.size() > 0);

    // All entries should have non-empty names and non-negative lengths
    for (const auto& e : entries)
    {
        REQUIRE(e.name.empty() == false);
        REQUIRE(e.length >= 0);
    }

    bank.Unlock();
}

TEST_CASE("FileBankManager - packed PBO entry names use backslash separators", "[qstream][pbo][roundtrip]")
{
    const fs::path root = fs::temp_directory_path() / "poseidon_pbo_separator_src";
    const fs::path nested = root / "adam" / "alphabet";
    const fs::path pbo = fs::temp_directory_path() / "poseidon_pbo_separator.pbo";

    std::error_code ec;
    fs::remove_all(root, ec);
    fs::remove(pbo, ec);
    fs::create_directories(nested);
    {
        std::ofstream out(nested / "alpha.wss", std::ios::binary);
        REQUIRE(out.good());
        out << "alpha";
    }

    FileBankManager manager;
    REQUIRE(manager.Create(pbo.string().c_str(), root.string().c_str()) == LSOK);

    const std::string firstName = ReadFirstPboEntryName(pbo);
    REQUIRE(firstName == "adam\\alphabet\\alpha.wss");
    REQUIRE(firstName.find('/') == std::string::npos);

    QFBank bank;
    const std::string bankName = pbo.string().substr(0, pbo.string().size() - 4);
    REQUIRE(bank.open(RString(bankName.c_str())));
    bank.Lock();
    REQUIRE_FALSE(bank.error());
    REQUIRE(bank.FileExists("adam\\alphabet\\alpha.wss"));
    Ref<IFileBuffer> data = bank.Read("adam\\alphabet\\alpha.wss");
    REQUIRE(data != nullptr);
    REQUIRE(data->GetSize() == 5);
    bank.Unlock();

    fs::remove(pbo, ec);
    fs::remove_all(root, ec);
}

TEST_CASE("QFBank - Open PBO fixture (addon_fixture)", "[qstream][pbo][roundtrip]")
{
    REQUIRE_FIXTURE("pbo/addon_fixture.pbo");

    std::string fixture = GetTestFixturePath("pbo/addon_fixture.pbo");
    std::string bankName = fixture.substr(0, fixture.size() - 4);

    QFBank bank;
    bool opened = bank.open(RString(bankName.c_str()));
    REQUIRE(opened);

    bank.Lock();
    REQUIRE(bank.error() == false);

    std::vector<PboEntry> entries;
    PboCollect ctx{&entries};
    bank.ForEach(CollectPboEntry, &ctx);

    REQUIRE(entries.size() > 0);

    bank.Unlock();
}

TEST_CASE("QFBank - FileExists on PBO", "[qstream][pbo][roundtrip]")
{
    REQUIRE_FIXTURE("pbo/mission_fixture.Intro.pbo");

    std::string fixture = GetTestFixturePath("pbo/mission_fixture.Intro.pbo");
    std::string bankName = fixture.substr(0, fixture.size() - 4);

    QFBank bank;
    REQUIRE(bank.open(RString(bankName.c_str())));
    bank.Lock();

    // Collect entries to get a known filename
    std::vector<PboEntry> entries;
    PboCollect ctx{&entries};
    bank.ForEach(CollectPboEntry, &ctx);
    REQUIRE(entries.size() > 0);

    // First entry should exist
    REQUIRE(bank.FileExists(entries[0].name.c_str()));

    // Non-existent file should not
    REQUIRE(bank.FileExists("__nonexistent_file_12345.txt") == false);

    bank.Unlock();
}

TEST_CASE("QFBank - Read file from PBO", "[qstream][pbo][roundtrip]")
{
    REQUIRE_FIXTURE("pbo/mission_fixture.Intro.pbo");

    std::string fixture = GetTestFixturePath("pbo/mission_fixture.Intro.pbo");
    std::string bankName = fixture.substr(0, fixture.size() - 4);

    QFBank bank;
    REQUIRE(bank.open(RString(bankName.c_str())));
    bank.Lock();

    // Get first entry
    std::vector<PboEntry> entries;
    PboCollect ctx{&entries};
    bank.ForEach(CollectPboEntry, &ctx);
    REQUIRE(entries.size() > 0);

    // Read the first file
    Ref<IFileBuffer> data = bank.Read(entries[0].name.c_str());
    REQUIRE(data != nullptr);
    REQUIRE(data->GetSize() > 0);
    REQUIRE(data->GetData() != nullptr);

    bank.Unlock();
}

TEST_CASE("QFBank - Read all files from PBO", "[qstream][pbo][roundtrip]")
{
    REQUIRE_FIXTURE("pbo/mission_fixture.Intro.pbo");

    std::string fixture = GetTestFixturePath("pbo/mission_fixture.Intro.pbo");
    std::string bankName = fixture.substr(0, fixture.size() - 4);

    QFBank bank;
    REQUIRE(bank.open(RString(bankName.c_str())));
    bank.Lock();

    std::vector<PboEntry> entries;
    PboCollect ctx{&entries};
    bank.ForEach(CollectPboEntry, &ctx);

    // Read every file and verify it's readable
    for (const auto& e : entries)
    {
        Ref<IFileBuffer> data = bank.Read(e.name.c_str());
        REQUIRE(data != nullptr);
        REQUIRE(data->GetSize() > 0);
        REQUIRE(data->GetData() != nullptr);

        // For compressed entries, verify decompressed size matches
        if (e.compressedMagic == CompMagic)
        {
            REQUIRE(data->GetSize() == e.uncompressedSize);
        }
        else
        {
            REQUIRE(data->GetSize() == e.length);
        }
    }

    bank.Unlock();
}

TEST_CASE("QFBank - Open non-existent PBO fails", "[qstream][pbo][roundtrip]")
{
    QFBank bank;
    bool opened = bank.open(RString("__nonexistent_pbo_12345"));
    REQUIRE(opened == false);
}

TEST_CASE("QFBank - GetProperty on PBO", "[qstream][pbo][roundtrip]")
{
    REQUIRE_FIXTURE("pbo/mission_fixture.Intro.pbo");

    std::string fixture = GetTestFixturePath("pbo/mission_fixture.Intro.pbo");
    std::string bankName = fixture.substr(0, fixture.size() - 4);

    QFBank bank;
    REQUIRE(bank.open(RString(bankName.c_str())));
    bank.Lock();

    // GetProperty on non-existent property should return empty
    const RString& val = bank.GetProperty(RString("nonexistent_property"));
    REQUIRE(val.GetLength() == 0);

    bank.Unlock();
}
