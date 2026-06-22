#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <stddef.h>

// I-33 / B-037 — TL emission timing.
//
// B-037 introduced a regression where the frame layer's end-of-frame `Execute`
// re-issued every captured TL draw.  At that point in the frame
// the legacy sky depth, shadow stencil, and darken quad had
// already landed, so the deferred draws fell behind state they
// no longer matched (horizon trees lost to sky depth, opaque
// objects painted over shadow modulation).  The fix moves the frame layer's
// `EmitDraw` inline at the `DrawSectionTL` callsite, where the
// per-draw state has just been established.
//
// This audit enforces:
//   1. `EngineGL33::EmitDraw` is called inline at `DrawSectionTL`
//      in `EngineGL33_VertexBuffer.cpp` — the descriptor-landed
//      site where emission belongs.
//   2. The end-of-frame validation layer (Graphics/Rendering/Frame)
//      contains no dispatch machinery and no emission call — the
//      replay path doesn't exist, so the B-037 ordering bug is
//      structurally unrepresentable.

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

std::filesystem::path RepoRoot()
{
    return std::filesystem::path(TESTS_ROOT_DIR).parent_path();
}
} // namespace

TEST_CASE("I-33: EmitDraw is called inline at DrawSectionTL", "[Rendering][Frame][EmissionTiming][I-33]")
{
    const std::string body = ReadTextFile(RepoRoot() / "engine" / "PoseidonGL33" / "EngineGL33_VertexBuffer.cpp");
    REQUIRE_FALSE(body.empty());

    // The `DrawSectionTL` function (the legacy state-landed site)
    // must contain exactly one `EmitDraw(d)` call.  More than one
    // would mean a duplicated path; zero means the emission
    // seam fell off the legacy path.
    const size_t fnStart = body.find("DrawSectionTL");
    REQUIRE(fnStart != std::string::npos);

    int inlineCalls = 0;
    size_t pos = 0;
    while ((pos = body.find("EmitDraw(d)", pos)) != std::string::npos)
    {
        ++inlineCalls;
        pos += 1;
    }
    REQUIRE(inlineCalls == 1);
}

TEST_CASE("I-33: the frame validator contains no replay dispatch", "[Rendering][Frame][EmissionTiming][I-33]")
{
    const std::filesystem::path frameDir = RepoRoot() / "engine" / "Poseidon" / "Graphics" / "Rendering" / "Frame";

    // The end-of-frame replay machinery (Execute / IFrameBackend /
    // backends) is deleted, not just unused — reintroducing a file
    // that walks the Frame against a backend re-opens the B-037
    // ordering bug.
    REQUIRE_FALSE(std::filesystem::exists(frameDir / "Execute.hpp"));
    REQUIRE_FALSE(std::filesystem::exists(frameDir / "Execute.cpp"));
    REQUIRE_FALSE(std::filesystem::exists(frameDir / "IFrameBackend.hpp"));
    REQUIRE_FALSE(std::filesystem::exists(frameDir / "EngineGL33Backend.cpp"));

    // And the validator itself never emits: no draw or pipeline call
    // sites anywhere in the observation layer.
    int scanned = 0;
    for (const auto& entry : std::filesystem::directory_iterator(frameDir))
    {
        const auto ext = entry.path().extension();
        if (ext != ".cpp" && ext != ".hpp")
            continue;
        ++scanned;
        const std::string body = ReadTextFile(entry.path());
        INFO(entry.path().filename().string());
        REQUIRE(body.find("EmitDraw(") == std::string::npos);
        REQUIRE(body.find("ApplyPipelineDescriptor(") == std::string::npos);
    }
    REQUIRE(scanned > 0);
}
