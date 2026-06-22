#include <catch2/catch_test_macros.hpp>

#include <Poseidon/IO/Filesystem/FileOps.hpp>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <iterator>
#include <system_error>
#include <Poseidon/Foundation/platform.hpp>

namespace fs = std::filesystem;

namespace
{

struct TempIoDir
{
    fs::path root;

    explicit TempIoDir(const char* label)
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        root = fs::temp_directory_path() / (std::string("poseidon_base_io_") + label + "_" + std::to_string(nonce));
        fs::create_directories(root);
    }

    ~TempIoDir()
    {
        std::error_code ec;
        fs::remove_all(root, ec);
    }
};

std::string readText(const fs::path& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

} // namespace

TEST_CASE("FileOps opens, writes, appends, and reports file existence", "[poseidon-base][io][fileops]")
{
    TempIoDir dir("fileops_basic");
    const fs::path outputPath = dir.root / "output.txt";
    const std::string outputPathString = outputPath.string();

    SECTION("open_write truncates and open_append preserves existing content")
    {
        HANDLE file = OpenFileForWrite(outputPathString.c_str(), true);
        REQUIRE(file != INVALID_HANDLE_VALUE);

        constexpr char initial[] = "Alpha";
        DWORD written = 0;
        REQUIRE(WriteFile(file, initial, std::strlen(initial), &written, nullptr));
        REQUIRE(written == std::strlen(initial));
        CloseHandle(file);

        file = OpenFileForAppend(outputPathString.c_str());
        REQUIRE(file != INVALID_HANDLE_VALUE);

        constexpr char suffix[] = "Beta";
        written = 0;
        REQUIRE(WriteFile(file, suffix, std::strlen(suffix), &written, nullptr));
        REQUIRE(written == std::strlen(suffix));
        CloseHandle(file);

        REQUIRE(readText(outputPath) == "AlphaBeta");
    }

    SECTION("open_read and file_size work on an existing file")
    {
        std::ofstream(outputPath, std::ios::binary) << "Payload";

        HANDLE file = OpenFileForRead(outputPathString.c_str());
        REQUIRE(file != INVALID_HANDLE_VALUE);
        REQUIRE(GetOpenFileSize(file) == 7);
        CloseHandle(file);
    }

    SECTION("path_exists distinguishes existing and missing paths")
    {
        std::ofstream(outputPath, std::ios::binary) << "x";
        REQUIRE(FilePathExists(outputPathString.c_str()));

        const std::string missingPath = (dir.root / "missing.txt").string();
        REQUIRE_FALSE(FilePathExists(missingPath.c_str()));
    }
}

#ifndef _WIN32
TEST_CASE("FileOps resolves mixed-case paths on POSIX", "[poseidon-base][io][fileops]")
{
    TempIoDir dir("fileops_ci");
    fs::create_directories(dir.root / "DTA");
    const fs::path actualPath = dir.root / "DTA" / "Config.BIN";
    std::ofstream(actualPath, std::ios::binary) << "CaseSensitive";

    const std::string mixedCasePath = (dir.root / "dta" / "config.bin").string();

    REQUIRE(FilePathExists(mixedCasePath.c_str()));

    HANDLE file = OpenFileForRead(mixedCasePath.c_str());
    REQUIRE(file != INVALID_HANDLE_VALUE);
    REQUIRE(GetOpenFileSize(file) == 13);
    CloseHandle(file);
}

// Regression: an empty wrong-case sibling directory must not shadow the populated real
// one. The single-mission launch builds a lowercase "missions\..." path while the data
// ships as "Missions/"; an empty lowercase "missions/" (auto-created elsewhere) used to
// make ci_resolve_path commit to the exact-case empty dir and report ENOENT, so the
// mission never loaded and the game dropped back to the menu. Without the backtracking
// fix this resolves to nothing; with it, the file in "Missions/" is found.
TEST_CASE("FileOps backtracks past an empty wrong-case sibling directory", "[poseidon-base][io][fileops]")
{
    TempIoDir dir("fileops_shadow");
    fs::create_directories(dir.root / "missions");                       // empty, lowercase — the shadow
    fs::create_directories(dir.root / "Missions" / "01TakeTheCar.ABEL"); // real, capitalised
    const fs::path actualPath = dir.root / "Missions" / "01TakeTheCar.ABEL" / "mission.sqm";
    std::ofstream(actualPath, std::ios::binary) << "version=11";

    const std::string lowercasePath = (dir.root / "missions" / "01TakeTheCar.ABEL" / "mission.sqm").string();

    REQUIRE(FilePathExists(lowercasePath.c_str()));

    HANDLE file = OpenFileForRead(lowercasePath.c_str());
    REQUIRE(file != INVALID_HANDLE_VALUE);
    REQUIRE(GetOpenFileSize(file) == 10);
    CloseHandle(file);
}
#endif
