// Audio single-owner / cache-stale audit — audio-invariants A-12.
//
// Pins the structural pattern that gameplay code outside the audio
// subsystem doesn't roll its own "is the cached IWave* stale?" check;
// every such check routes through audio::IsCacheStale (or the
// RetriggerCachedWave helper on top of it).  A regression that
// reintroduces a raw `wave->IsTerminated()` check on a cached
// IWave* in gameplay code would surface the next IWave FSM extension
// (Failed-load as a distinct state, hardware-loss-revives-to-Created,
// etc.) without updating callers — exactly the GL-12 / B-A02 shape.
//
// The audit is source-grep based.  Internal audio sites (the budget
// loop, SoundScene cleanup, diagnostic functions) are exempt — they
// own the live FSM and can read it directly.

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stddef.h>
#include <catch2/catch_message.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>

using namespace Poseidon;
namespace
{

std::string ReadTextFile(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::filesystem::path EngineRoot()
{
    return std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "Poseidon";
}

} // namespace

TEST_CASE("A-12: EntityAI::WeaponSoundCacheStale routes through audio::IsCacheStale", "[Audio][cache-audit][A-12]")
{
    const std::string src = ReadTextFile(EngineRoot() / "AI" / "VehicleAICombat.cpp");
    REQUIRE_FALSE(src.empty());

    // The implementation must reference the named helper, not roll
    // its own `wave->IsTerminated()` check.
    CHECK(src.find("audio::IsCacheStale") != std::string::npos);

    // The WeaponSoundCacheStale body must not contain "IsTerminated"
    // (the legacy implementation pre-Phase-E.3).  Find the function
    // body and inspect just that range.
    const std::string proto = "bool EntityAI::WeaponSoundCacheStale";
    const auto p = src.find(proto);
    REQUIRE(p != std::string::npos);
    const auto openBrace = src.find('{', p);
    REQUIRE(openBrace != std::string::npos);
    // Find the matching close brace.
    int depth = 1;
    size_t closeBrace = openBrace + 1;
    for (; closeBrace < src.size() && depth > 0; ++closeBrace)
    {
        if (src[closeBrace] == '{')
            ++depth;
        else if (src[closeBrace] == '}')
            --depth;
    }
    REQUIRE(depth == 0);
    const std::string body = src.substr(openBrace, closeBrace - openBrace);

    CHECK(body.find("IsTerminated") == std::string::npos);
    CHECK(body.find("audio::IsCacheStale") != std::string::npos);
}

// audio-invariants A-15 — preview lifecycle parity with gameplay.
//
// Every preview wave (StartPreview / StartCategoryPreview /
// StartDevicePreview / StartEAXPreview) must be a regular IWave
// driven via standard verbs (Play / Stop / Pause / Resume / SetVolume
// / Repeat / SetPosition / SetSticky / SetKind / EnableAccommodation).
// No preview-only side-channels — no raw alSourceStop / alSourcePlay /
// alDeleteSources on preview-wave handles bypassing the IWave layer.
//
// The audit greps the SoundSystemOAL implementation for any
// `_preview*` usage that crosses into AL state directly.  All AL
// calls in the file are legitimate (UpdateMixer's listener gain,
// context init) — they should NOT touch _previewSpeech /
// _previewEffect / _previewMusic / _previewCategory / _previewDevice
// / _previewEax.
TEST_CASE("A-15: preview waves driven via standard IWave verbs only", "[Audio][cache-audit][A-15]")
{
    const std::string src = ReadTextFile(std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" /
                                         "PoseidonOpenAL" / "SoundSystemOAL.cpp");
    REQUIRE_FALSE(src.empty());

    const std::vector<std::string> previewMembers = {"_previewSpeech",   "_previewEffect", "_previewMusic",
                                                     "_previewCategory", "_previewDevice", "_previewEax"};

    int previewTouchLines = 0;
    int forbiddenOnPreview = 0;
    std::vector<std::string> bad;

    std::stringstream stream(src);
    std::string line;
    while (std::getline(stream, line))
    {
        bool touchesPreview = false;
        for (const auto& m : previewMembers)
        {
            if (line.find(m) != std::string::npos)
            {
                touchesPreview = true;
                break;
            }
        }
        if (!touchesPreview)
            continue;
        ++previewTouchLines;

        if (line.find("alSource") != std::string::npos || line.find("alBuffer") != std::string::npos ||
            line.find("alDelete") != std::string::npos || line.find("alGen") != std::string::npos)
        {
            ++forbiddenOnPreview;
            bad.push_back(line);
        }
    }

    INFO("Preview-touching lines: " << previewTouchLines);
    for (const auto& b : bad)
    {
        INFO("Forbidden: " << b);
    }
    CHECK(forbiddenOnPreview == 0);
    // Sanity: the file actually touches preview waves (the audit isn't
    // running against an empty namespace).
    CHECK(previewTouchLines > 10);
}

TEST_CASE("A-12: audio::IsCacheStale / RetriggerCachedWave live in one named helper file", "[Audio][cache-audit][A-12]")
{
    // audio::IsCacheStale / RetriggerCachedWave is the only sanctioned
    // gameplay-side cache-reuse path.  Any
    // duplicate definitions in other headers would let regressions
    // diverge.  Audit: assert exactly one definition of each helper
    // across the Audio/ subtree.
    const std::filesystem::path audioDir = EngineRoot() / "Audio";
    REQUIRE(std::filesystem::is_directory(audioDir));

    int isCacheStaleDefs = 0;
    int retriggerDefs = 0;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(audioDir))
    {
        if (!entry.is_regular_file())
            continue;
        const auto ext = entry.path().extension();
        if (ext != ".hpp" && ext != ".cpp")
            continue;
        const std::string body = ReadTextFile(entry.path());
        // Match the inline definition signature, not callsites.
        if (body.find("inline bool IsCacheStale(IWave*") != std::string::npos)
            ++isCacheStaleDefs;
        if (body.find("inline bool RetriggerCachedWave(IWave*") != std::string::npos)
            ++retriggerDefs;
    }
    CHECK(isCacheStaleDefs == 1);
    CHECK(retriggerDefs == 1);
}
