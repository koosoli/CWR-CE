#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

// KHR_debug pass groups bracket the draws they name.
//
// The pass debug groups are emitted at the REAL pass transitions
// (`EngineGL33::BeginPass` / `BeginScreenPass`) and closed before the
// swap (`FinishDraw`), so a RenderDoc / Nsight capture shows each
// group around the GL calls of that pass.  Emitting them after the
// frame instead (from the end-of-frame validator) would bracket
// nothing — the frame's draws have already been issued — so the
// validation layer must contain no debug-group calls.

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

std::string ExtractFunctionBody(const std::string& src, const std::string& signature)
{
    const size_t sigPos = src.find(signature);
    if (sigPos == std::string::npos)
        return {};

    const size_t bodyStart = src.find('{', sigPos);
    if (bodyStart == std::string::npos)
        return {};

    int depth = 1;
    for (size_t i = bodyStart + 1; i < src.size(); ++i)
    {
        if (src[i] == '{')
            ++depth;
        else if (src[i] == '}')
        {
            --depth;
            if (depth == 0)
                return src.substr(bodyStart, i - bodyStart + 1);
        }
    }
    return {};
}

std::filesystem::path RepoRoot()
{
    return std::filesystem::path(TESTS_ROOT_DIR).parent_path();
}

std::filesystem::path Gl33Dir()
{
    return RepoRoot() / "engine" / "PoseidonGL33";
}

int CountOccurrences(const std::string& haystack, const std::string& needle)
{
    int count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos)
    {
        ++count;
        pos += needle.size();
    }
    return count;
}

} // namespace

TEST_CASE("Pass debug groups: switched at both BeginPass branches", "[Graphics][GL33][DebugGroup]")
{
    const std::string body = ReadTextFile(Gl33Dir() / "EngineGL33_Queue.cpp");
    REQUIRE_FALSE(body.empty());

    // One switch in the already-in-3D pass-change branch, one in the
    // full-init branch — both transitions must (re)name the group.
    const std::string fn = ExtractFunctionBody(body, "void EngineGL33::BeginPass(PassId passId)");
    REQUIRE_FALSE(fn.empty());
    REQUIRE(CountOccurrences(fn, "SwitchPassDebugGroup(") == 2);
}

TEST_CASE("Pass debug groups: BeginScreenPass opens the ScreenSpace group", "[Graphics][GL33][DebugGroup]")
{
    const std::string body = ReadTextFile(Gl33Dir() / "EngineGL33_Queue.cpp");
    REQUIRE_FALSE(body.empty());

    const std::string fn = ExtractFunctionBody(body, "void EngineGL33::BeginScreenPass()");
    REQUIRE_FALSE(fn.empty());
    REQUIRE(CountOccurrences(fn, "SwitchPassDebugGroup(") == 1);
}

TEST_CASE("Pass debug groups: FinishDraw closes the open group before the swap", "[Graphics][GL33][DebugGroup]")
{
    const std::string body = ReadTextFile(Gl33Dir() / "EngineGL33_Lifecycle.cpp");
    REQUIRE_FALSE(body.empty());

    const std::string fn = ExtractFunctionBody(body, "void EngineGL33::FinishDraw()");
    REQUIRE_FALSE(fn.empty());
    REQUIRE(CountOccurrences(fn, "ClosePassDebugGroup(") == 1);
}

TEST_CASE("Pass debug groups: the frame validator emits none", "[Graphics][GL33][DebugGroup]")
{
    // Post-hoc groups from the end-of-frame validator would wrap
    // nothing — the frame's draws have already been issued.  The whole
    // observation layer must stay free of debug-group calls.
    const std::filesystem::path frameDir = RepoRoot() / "engine" / "Poseidon" / "Graphics" / "Rendering" / "Frame";
    int scanned = 0;
    for (const auto& entry : std::filesystem::directory_iterator(frameDir))
    {
        const auto ext = entry.path().extension();
        if (ext != ".cpp" && ext != ".hpp")
            continue;
        ++scanned;
        const std::string body = ReadTextFile(entry.path());
        INFO(entry.path().filename().string());
        REQUIRE(body.find("BeginDebugGroup") == std::string::npos);
        REQUIRE(body.find("EndDebugGroup") == std::string::npos);
    }
    REQUIRE(scanned > 0);
}

TEST_CASE("Pass debug groups: every switch path is balanced", "[Graphics][GL33][DebugGroup]")
{
    // SwitchPassDebugGroup ends the open group before beginning the
    // next, and ClosePassDebugGroup is idempotent — the begin/end
    // pairing lives in exactly these two helpers, nowhere else in the
    // backend.  Raw Begin/EndDebugGroup outside the helpers (and the
    // virtual overrides themselves in EngineGL33_State.cpp) would
    // bypass the open-flag bookkeeping and unbalance the GL stack.
    int rawBegins = 0;
    int rawEnds = 0;
    for (const auto& entry : std::filesystem::directory_iterator(Gl33Dir()))
    {
        if (entry.path().extension() != ".cpp")
            continue;
        const std::string body = ReadTextFile(entry.path());
        rawBegins += CountOccurrences(body, "BeginDebugGroup(");
        rawEnds += CountOccurrences(body, "EndDebugGroup(");
    }
    // EngineGL33_State.cpp: the two virtual override definitions plus
    // the single call of each inside the Switch/Close helpers.
    REQUIRE(rawBegins == 2);
    REQUIRE(rawEnds == 3); // definition + Switch + Close
}
