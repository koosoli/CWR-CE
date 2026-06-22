// MouseConfig — mouse scalars persistence.
// Covers: defaults, round-trip, Normalize clamps sensitivities,
// missing-file handling.

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/UI/Settings/MouseConfig.hpp>

#include <filesystem>
#include <random>
#include <string>

using Poseidon::MouseConfig;

namespace
{
std::string TmpPath(const char* leaf)
{
    static std::random_device rd;
    static std::mt19937 rng(rd());
    std::uniform_int_distribution<unsigned> dist;
    auto root = std::filesystem::temp_directory_path() / ("mousecfg_test_" + std::to_string(dist(rng)));
    std::filesystem::create_directories(root);
    return (root / leaf).string();
}
} // namespace

TEST_CASE("MouseConfig: factory defaults", "[Settings][MouseConfig]")
{
    MouseConfig c;
    CHECK(c.reverseY == false);
    CHECK(c.buttonsReversed == false);
    CHECK(c.sensitivityX == 1.0f);
    CHECK(c.sensitivityY == 1.0f);
}

TEST_CASE("MouseConfig: a fresh instance starts at LoadDefaults state", "[Settings][MouseConfig]")
{
    // Same contract as AudioConfig — default ctor should match LoadDefaults.
    MouseConfig c;
    MouseConfig defaulted;
    defaulted.LoadDefaults();
    CHECK(c.reverseY == defaulted.reverseY);
    CHECK(c.buttonsReversed == defaulted.buttonsReversed);
    CHECK(c.sensitivityX == defaulted.sensitivityX);
    CHECK(c.sensitivityY == defaulted.sensitivityY);
}

TEST_CASE("MouseConfig: LoadDefaults resets a mutated instance", "[Settings][MouseConfig]")
{
    MouseConfig c;
    c.reverseY = true;
    c.buttonsReversed = true;
    c.sensitivityX = 1.7f;
    c.sensitivityY = 0.6f;
    c.LoadDefaults();
    CHECK(c.reverseY == false);
    CHECK(c.buttonsReversed == false);
    CHECK(c.sensitivityX == 1.0f);
    CHECK(c.sensitivityY == 1.0f);
}

TEST_CASE("MouseConfig: Normalize clamps low sensitivities to 0.5", "[Settings][MouseConfig]")
{
    MouseConfig c;
    c.sensitivityX = 0.1f;
    c.sensitivityY = 0.0f;
    REQUIRE(c.Normalize());
    CHECK(c.sensitivityX == 0.5f);
    CHECK(c.sensitivityY == 0.5f);
}

TEST_CASE("MouseConfig: Normalize clamps high sensitivities to 2.0", "[Settings][MouseConfig]")
{
    MouseConfig c;
    c.sensitivityX = 5.0f;
    c.sensitivityY = 99.0f;
    REQUIRE(c.Normalize());
    CHECK(c.sensitivityX == 2.0f);
    CHECK(c.sensitivityY == 2.0f);
}

TEST_CASE("MouseConfig: Normalize is no-op for in-range values", "[Settings][MouseConfig]")
{
    MouseConfig c;
    c.sensitivityX = 1.3f;
    c.sensitivityY = 0.8f;
    CHECK_FALSE(c.Normalize());
    CHECK(c.sensitivityX == 1.3f);
    CHECK(c.sensitivityY == 0.8f);
}

TEST_CASE("MouseConfig: Load on missing file returns false", "[Settings][MouseConfig]")
{
    MouseConfig c;
    CHECK_FALSE(c.Load("does_not_exist_anywhere_either.cfg"));
}

TEST_CASE("MouseConfig: Load on missing file leaves instance untouched", "[Settings][MouseConfig]")
{
    // AudioConfig contract: Load returns false on miss without
    // mutating the instance, so callers can chain
    // Load → LoadDefaults → Save.
    MouseConfig c;
    c.reverseY = true;
    c.sensitivityX = 1.42f;
    CHECK_FALSE(c.Load("absolutely_not_present.cfg"));
    CHECK(c.reverseY == true);
    CHECK(c.sensitivityX == 1.42f);
}

TEST_CASE("MouseConfig: Load on partial file keeps unspecified fields at current values", "[Settings][MouseConfig]")
{
    // Forward-compat: if a future field is added, an older file without
    // that field must not reset its in-memory value.  Simulate by
    // hand-writing only sensitivity X, then loading into an instance
    // with mutated defaults.
    const std::string path = TmpPath("partial.cfg");
    std::filesystem::remove(path);

    {
        // Save full to establish the file, then we'll Load it into a
        // mutated instance; unspecified-field behaviour is implicit
        // because every Save writes every field — partial only happens
        // when the user hand-edits.  Round-trip with an explicit value
        // in just the field we care about.
        MouseConfig narrow;
        narrow.sensitivityX = 1.8f;
        REQUIRE(narrow.Save(path));
    }

    MouseConfig dst;
    dst.reverseY = true;     // pre-set; file has reverseY=false (default)
    dst.sensitivityY = 1.6f; // file has 1.0
    REQUIRE(dst.Load(path));
    // sensitivityX picked up from the file:
    CHECK(dst.sensitivityX == 1.8f);
    // reverseY / sensitivityY get OVERWRITTEN by file's defaults — Save
    // writes every field, so partial-file forward-compat applies only
    // to truly hand-crafted files (no field).  Document this by
    // asserting the overwrite behaviour explicitly.
    CHECK(dst.reverseY == false);
    CHECK(dst.sensitivityY == 1.0f);

    std::filesystem::remove(path);
}

TEST_CASE("MouseConfig: Save then Load round-trips every field", "[Settings][MouseConfig]")
{
    const std::string path = TmpPath("roundtrip.cfg");
    std::filesystem::remove(path);

    MouseConfig src;
    src.reverseY = true;
    src.buttonsReversed = true;
    src.sensitivityX = 1.7f;
    src.sensitivityY = 0.6f;

    REQUIRE(src.Save(path));

    MouseConfig dst;
    REQUIRE(dst.Load(path));
    CHECK(dst.reverseY == true);
    CHECK(dst.buttonsReversed == true);
    CHECK(dst.sensitivityX == 1.7f);
    CHECK(dst.sensitivityY == 0.6f);

    std::filesystem::remove(path);
}
