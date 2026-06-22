#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>

// Spec-bit reduction — the backend's raw-bit boundary.
//
// `BuildRenderPassDescriptor` is the translation seam: legacy spec bits
// go in, a complete `RenderPassDescriptor` comes out, and the backend
// binds descriptor fields.  A handful of backend sites still read spec
// bits directly because they run before any per-draw descriptor exists
// (per-mesh light_disc gate, material cache key, multitexture stage selection)
// or because the backend produces its own 2D-quad specs.  Each is
// imported metadata, not state derivation.
//
// This audit pins that whitelist.  A new direct bit read anywhere in
// `engine/PoseidonGL33/*.cpp` fails the test — the boundary can only
// shrink.

namespace
{

std::string ReadStripped(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f.is_open())
        return {};
    std::stringstream ss;
    ss << f.rdbuf();
    // Strip line comments so prose mentioning bit names doesn't count.
    return std::regex_replace(ss.str(), std::regex("//[^\n]*"), "");
}

std::filesystem::path Gl33Dir()
{
    return std::filesystem::path(TESTS_ROOT_DIR).parent_path() / "engine" / "PoseidonGL33";
}

} // namespace

TEST_CASE("Spec-bit boundary: typed spec reads in the backend match the whitelist",
          "[Graphics][GL33][SpecSplit][boundary]")
{
    // file -> allowed `Category::Bit` tokens.  `::None` is a sentinel,
    // not a bit, and is allowed everywhere.
    const std::map<std::string, std::set<std::string>> kAllowed = {
        // Per-mesh light_disc gate + material cache key — runs at SetMaterial
        // time, before any per-section descriptor exists.
        {"EngineGL33_Material.cpp", {"Material::DisableSun"}},
        // Per-mesh light_disc enable + per-object constColor upload at
        // PrepareMeshTL time.
        {"EngineGL33_Mesh.cpp", {"Material::DisableSun", "Routing::IsColored"}},
        // Multitexture stage selection in SetTexture — slot-1 binding
        // decision, upstream of the descriptor.
        {"EngineGL33_State.cpp", {"Backend::DetailTexture", "Backend::SpecularTexture", "Backend::GrassTexture"}},
    };

    const std::regex tokenRe("\\b(Routing|Material|Backend)::([A-Za-z]+)\\b");
    for (const auto& entry : std::filesystem::directory_iterator(Gl33Dir()))
    {
        if (entry.path().extension() != ".cpp")
            continue;
        const std::string file = entry.path().filename().string();
        const std::string body = ReadStripped(entry.path());

        std::set<std::string> allowed;
        const auto it = kAllowed.find(file);
        if (it != kAllowed.end())
            allowed = it->second;

        for (std::sregex_iterator m(body.begin(), body.end(), tokenRe), e; m != e; ++m)
        {
            const std::string token = (*m)[1].str() + "::" + (*m)[2].str();
            if ((*m)[2].str() == "None")
                continue;
            INFO(file << " reads " << token);
            REQUIRE(allowed.count(token) == 1);
        }
    }
}

TEST_CASE("Spec-bit boundary: raw legacy bit tokens confined to the 2D producers",
          "[Graphics][GL33][SpecSplit][boundary]")
{
    // Raw (un-namespaced) legacy bit macros in backend code.  The 2D
    // path builds its own quad specs (producer, flows into the
    // translation at queue flush); the shared 2D prepare keys one
    // branch off IsAlphaFog.  Method calls like `tex->IsAlpha()` are
    // excluded by the no-paren rule.
    const std::map<std::string, std::set<std::string>> kAllowed = {
        {"EngineGL33_2D.cpp", {"NoZBuf", "NoZWrite", "IsAlpha", "ClampU", "ClampV", "IsAlphaFog"}},
        {"EngineGL33_DrawShared.cpp", {"IsAlphaFog"}},
    };
    const char* kBits[] = {"NoZBuf",     "NoZWrite",    "IsShadow",  "IsAlpha",       "IsTransparent",  "IsWater",
                           "IsLight",    "IsAlphaFog",  "ClampU",    "ClampV",        "NoClamp",        "PointSampling",
                           "NoDropdown", "FogDisabled", "IsColored", "DetailTexture", "SpecularTexture"};

    for (const auto& entry : std::filesystem::directory_iterator(Gl33Dir()))
    {
        if (entry.path().extension() != ".cpp")
            continue;
        const std::string file = entry.path().filename().string();
        const std::string body = ReadStripped(entry.path());

        std::set<std::string> allowed;
        const auto it = kAllowed.find(file);
        if (it != kAllowed.end())
            allowed = it->second;

        for (const char* bit : kBits)
        {
            // Bare token: no `::` qualifier before, no `(` after (method
            // calls), no alphanumeric neighbours.
            const std::regex bare(std::string("(?:^|[^:\\w])") + bit + "(?![\\w(])");
            for (std::sregex_iterator m(body.begin(), body.end(), bare), e; m != e; ++m)
            {
                INFO(file << " uses raw bit token " << bit);
                REQUIRE(allowed.count(bit) == 1);
            }
        }
    }
}
