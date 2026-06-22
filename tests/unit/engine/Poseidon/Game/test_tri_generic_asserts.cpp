#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <Evaluator/express.hpp>
#include <Poseidon/Foundation/Modules/Modules.hpp>
#include <string>

using namespace Poseidon;
using namespace Catch::Matchers;

// Forward declarations — forces GameStateExtTest.cpp into the link.
// GameStateExtTestAudio.cpp is pulled in because its INIT_MODULE(GameStateExtTest,3)
// references TriAssert etc. defined here.  Its static GRegisterModuleGameStateExtTest
// then self-registers at startup so InitModules() can wire up the verbs.
GameValue TriAssert(const GameState*, GameValuePar);
GameValue TriRefute(const GameState*, GameValuePar);
GameValue TriAssertEq(const GameState*, GameValuePar);
GameValue TriAssertNe(const GameState*, GameValuePar);
GameValue TriAssertGt(const GameState*, GameValuePar);
GameValue TriAssertGe(const GameState*, GameValuePar);
GameValue TriAssertLt(const GameState*, GameValuePar);
GameValue TriAssertLe(const GameState*, GameValuePar);
GameValue TriAssertNear(const GameState*, GameValuePar);
GameValue TriAssertChanged(const GameState*, GameValuePar);
GameValue TriAssertIncludes(const GameState*, GameValuePar);
GameValue TriAssertExcludes(const GameState*, GameValuePar);
GameValue TriAssertMatches(const GameState*, GameValuePar);

namespace
{

// GGameState.Init() registers the type table (SCALAR/ARRAY/STRING/…) so that
// CreateGameValue(GameArray) allocates a real GameDataArray.  Without Init()
// the type table is empty and CreateGameValue returns a null-typed GameValue.
struct InitFixture
{
    InitFixture()
    {
        static bool done = false;
        if (!done)
        {
            GGameState.Init();
            done = true;
        }
    }
};

static std::string S(const GameValue& v)
{
    return std::string(((RString)(GameStringType)v).Data());
}

static GameValue Arr1(GameValue v0)
{
    GameValue arr = GGameState.CreateGameValue(GameArray);
    GameArrayType& a = arr;
    a.Resize(1);
    a[0] = v0;
    return arr;
}

static GameValue Arr2(GameValue v0, GameValue v1)
{
    GameValue arr = GGameState.CreateGameValue(GameArray);
    GameArrayType& a = arr;
    a.Resize(2);
    a[0] = v0;
    a[1] = v1;
    return arr;
}

static GameValue Arr3(GameValue v0, GameValue v1, GameValue v2)
{
    GameValue arr = GGameState.CreateGameValue(GameArray);
    GameArrayType& a = arr;
    a.Resize(3);
    a[0] = v0;
    a[1] = v1;
    a[2] = v2;
    return arr;
}

static GameValue GStr(const char* s)
{
    return GameValue(RString(s));
}
static GameValue GScalar(float f)
{
    return GameValue(f);
}

} // namespace

// ============================================================================
// triAssert / triRefute
// ============================================================================

TEST_CASE("triAssert - passes on truthy, fails on falsy", "[tri][generic-assert][triAssert]")
{
    InitFixture fix;
    REQUIRE(S(TriAssert(nullptr, Arr1(GScalar(1)))) == "OK");
    REQUIRE(S(TriAssert(nullptr, Arr1(GScalar(42)))) == "OK");
    REQUIRE_THAT(S(TriAssert(nullptr, Arr1(GScalar(0)))), StartsWith("FAIL:expected truthy"));
    REQUIRE_THAT(S(TriAssert(nullptr, Arr1(GScalar(0)))), ContainsSubstring("got 0"));
}

TEST_CASE("triRefute - passes on falsy, fails on truthy", "[tri][generic-assert][triRefute]")
{
    InitFixture fix;
    REQUIRE(S(TriRefute(nullptr, Arr1(GScalar(0)))) == "OK");
    REQUIRE_THAT(S(TriRefute(nullptr, Arr1(GScalar(1)))), StartsWith("FAIL:expected falsy"));
    REQUIRE_THAT(S(TriRefute(nullptr, Arr1(GScalar(1)))), ContainsSubstring("got 1"));
}

// ============================================================================
// triAssertEq / triAssertNe
// ============================================================================

TEST_CASE("triAssertEq - scalar equality", "[tri][generic-assert][triAssertEq]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertEq(nullptr, Arr2(GScalar(3), GScalar(3)))) == "OK");
    REQUIRE(S(TriAssertEq(nullptr, Arr2(GScalar(0), GScalar(0)))) == "OK");
    REQUIRE_THAT(S(TriAssertEq(nullptr, Arr2(GScalar(3), GScalar(4)))), StartsWith("FAIL:expected =="));
    REQUIRE_THAT(S(TriAssertEq(nullptr, Arr2(GScalar(3), GScalar(4)))), ContainsSubstring("got 3"));
    REQUIRE_THAT(S(TriAssertEq(nullptr, Arr2(GScalar(3), GScalar(4)))), ContainsSubstring("4"));
}

TEST_CASE("triAssertEq - string equality", "[tri][generic-assert][triAssertEq]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertEq(nullptr, Arr2(GStr("windowed"), GStr("windowed")))) == "OK");
    REQUIRE_THAT(S(TriAssertEq(nullptr, Arr2(GStr("windowed"), GStr("fullscreen")))), StartsWith("FAIL:expected =="));
    REQUIRE_THAT(S(TriAssertEq(nullptr, Arr2(GStr("windowed"), GStr("fullscreen")))), ContainsSubstring("windowed"));
}

TEST_CASE("triAssertNe - scalar inequality", "[tri][generic-assert][triAssertNe]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertNe(nullptr, Arr2(GScalar(3), GScalar(4)))) == "OK");
    REQUIRE_THAT(S(TriAssertNe(nullptr, Arr2(GScalar(3), GScalar(3)))), StartsWith("FAIL:expected !="));
    REQUIRE_THAT(S(TriAssertNe(nullptr, Arr2(GScalar(3), GScalar(3)))), ContainsSubstring("got 3"));
}

// ============================================================================
// triAssertGt / triAssertGe / triAssertLt / triAssertLe
// ============================================================================

TEST_CASE("triAssertGt", "[tri][generic-assert][triAssertGt]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertGt(nullptr, Arr2(GScalar(5), GScalar(3)))) == "OK");
    REQUIRE_THAT(S(TriAssertGt(nullptr, Arr2(GScalar(3), GScalar(3)))), StartsWith("FAIL:expected >"));
    REQUIRE_THAT(S(TriAssertGt(nullptr, Arr2(GScalar(2), GScalar(3)))), StartsWith("FAIL:expected >"));
    REQUIRE_THAT(S(TriAssertGt(nullptr, Arr2(GScalar(2), GScalar(3)))), ContainsSubstring("got 2"));
    REQUIRE_THAT(S(TriAssertGt(nullptr, Arr2(GScalar(2), GScalar(3)))), ContainsSubstring("3"));
}

TEST_CASE("triAssertGe", "[tri][generic-assert][triAssertGe]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertGe(nullptr, Arr2(GScalar(3), GScalar(3)))) == "OK");
    REQUIRE(S(TriAssertGe(nullptr, Arr2(GScalar(4), GScalar(3)))) == "OK");
    REQUIRE_THAT(S(TriAssertGe(nullptr, Arr2(GScalar(2), GScalar(3)))), StartsWith("FAIL:expected >="));
    REQUIRE_THAT(S(TriAssertGe(nullptr, Arr2(GScalar(2), GScalar(3)))), ContainsSubstring("got 2"));
}

TEST_CASE("triAssertLt", "[tri][generic-assert][triAssertLt]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertLt(nullptr, Arr2(GScalar(2), GScalar(3)))) == "OK");
    REQUIRE_THAT(S(TriAssertLt(nullptr, Arr2(GScalar(3), GScalar(3)))), StartsWith("FAIL:expected <"));
    REQUIRE_THAT(S(TriAssertLt(nullptr, Arr2(GScalar(4), GScalar(3)))), StartsWith("FAIL:expected <"));
    REQUIRE_THAT(S(TriAssertLt(nullptr, Arr2(GScalar(4), GScalar(3)))), ContainsSubstring("got 4"));
}

TEST_CASE("triAssertLe", "[tri][generic-assert][triAssertLe]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertLe(nullptr, Arr2(GScalar(3), GScalar(3)))) == "OK");
    REQUIRE(S(TriAssertLe(nullptr, Arr2(GScalar(2), GScalar(3)))) == "OK");
    REQUIRE_THAT(S(TriAssertLe(nullptr, Arr2(GScalar(4), GScalar(3)))), StartsWith("FAIL:expected <="));
    REQUIRE_THAT(S(TriAssertLe(nullptr, Arr2(GScalar(4), GScalar(3)))), ContainsSubstring("got 4"));
}

// ============================================================================
// triAssertNear -- scalar mode
// ============================================================================

TEST_CASE("triAssertNear - scalar passes within tolerance", "[tri][generic-assert][triAssertNear]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GScalar(5), GScalar(5), GScalar(0)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GScalar(5.3f), GScalar(5), GScalar(0.5f)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GScalar(4.7f), GScalar(5), GScalar(0.5f)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GScalar(5.5f), GScalar(5), GScalar(0.5f)))) == "OK");
}

TEST_CASE("triAssertNear - scalar fails outside tolerance", "[tri][generic-assert][triAssertNear]")
{
    InitFixture fix;
    std::string r = S(TriAssertNear(nullptr, Arr3(GScalar(6), GScalar(5), GScalar(0.5f))));
    REQUIRE_THAT(r, StartsWith("FAIL:expected near"));
    REQUIRE_THAT(r, ContainsSubstring("got 6"));
    REQUIRE_THAT(r, ContainsSubstring("5"));
}

// ============================================================================
// triAssertNear -- RGB mode ("R,G,B" strings with 2 commas)
// ============================================================================

TEST_CASE("triAssertNear - RGB passes within tolerance", "[tri][generic-assert][triAssertNear]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("128,64,32"), GStr("128,64,32"), GScalar(0)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("130,64,32"), GStr("128,64,32"), GScalar(4)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("128,68,32"), GStr("128,64,32"), GScalar(4)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("128,64,36"), GStr("128,64,32"), GScalar(4)))) == "OK");
}

TEST_CASE("triAssertNear - RGB fails when any channel exceeds tolerance", "[tri][generic-assert][triAssertNear]")
{
    InitFixture fix;
    std::string r = S(TriAssertNear(nullptr, Arr3(GStr("136,64,32"), GStr("128,64,32"), GScalar(4))));
    REQUIRE_THAT(r, StartsWith("FAIL:expected near"));
    REQUIRE_THAT(r, ContainsSubstring("dr=8"));
    REQUIRE_THAT(r, ContainsSubstring("dg=0"));
    REQUIRE_THAT(r, ContainsSubstring("db=0"));
}

// ============================================================================
// triAssertNear -- 2D float mode ("x,y" strings with 1 comma)
// ============================================================================

TEST_CASE("triAssertNear - 2D float mode", "[tri][generic-assert][triAssertNear]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("0.5,0.5"), GStr("0.5,0.5"), GScalar(0.01f)))) == "OK");
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("0.51,0.5"), GStr("0.5,0.5"), GScalar(0.02f)))) == "OK");
    REQUIRE_THAT(S(TriAssertNear(nullptr, Arr3(GStr("0.6,0.5"), GStr("0.5,0.5"), GScalar(0.05f)))),
                 StartsWith("FAIL:expected near"));
}

// ============================================================================
// triAssertNear -- 4D float mode ("x,y,w,h" strings with 3 commas)
// ============================================================================

TEST_CASE("triAssertNear - 4D float mode", "[tri][generic-assert][triAssertNear]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertNear(nullptr, Arr3(GStr("10,20,30,40"), GStr("10,20,30,40"), GScalar(0.5f)))) == "OK");
    REQUIRE_THAT(S(TriAssertNear(nullptr, Arr3(GStr("10,20,30,50"), GStr("10,20,30,40"), GScalar(5)))),
                 StartsWith("FAIL:expected near"));
}

// ============================================================================
// triAssertChanged -- RGB change detection
// ============================================================================

TEST_CASE("triAssertChanged - detects change above minDelta", "[tri][generic-assert][triAssertChanged]")
{
    InitFixture fix;
    // Red channel differs by 5 -- >= minDelta 4
    REQUIRE(S(TriAssertChanged(nullptr, Arr3(GStr("105,100,100"), GStr("100,100,100"), GScalar(4)))) == "OK");
    // Green channel differs by 4 -- >= minDelta 4
    REQUIRE(S(TriAssertChanged(nullptr, Arr3(GStr("100,104,100"), GStr("100,100,100"), GScalar(4)))) == "OK");
}

TEST_CASE("triAssertChanged - fails when no channel meets minDelta", "[tri][generic-assert][triAssertChanged]")
{
    InitFixture fix;
    // Red differs by 3 only -- < minDelta 4
    std::string r = S(TriAssertChanged(nullptr, Arr3(GStr("103,100,100"), GStr("100,100,100"), GScalar(4))));
    REQUIRE_THAT(r, StartsWith("FAIL:expected changed by >= 4"));
    REQUIRE_THAT(r, ContainsSubstring("dr=3"));

    // No change
    REQUIRE_THAT(S(TriAssertChanged(nullptr, Arr3(GStr("100,100,100"), GStr("100,100,100"), GScalar(1)))),
                 StartsWith("FAIL:expected changed"));
}

// ============================================================================
// triAssertIncludes / triAssertExcludes / triAssertMatches
// ============================================================================

TEST_CASE("triAssertIncludes - substring check", "[tri][generic-assert][triAssertIncludes]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertIncludes(nullptr, Arr2(GStr("hello world"), GStr("world")))) == "OK");
    REQUIRE(S(TriAssertIncludes(nullptr, Arr2(GStr("hello world"), GStr("hello")))) == "OK");
    REQUIRE_THAT(S(TriAssertIncludes(nullptr, Arr2(GStr("hello world"), GStr("xyz")))),
                 StartsWith("FAIL:expected includes xyz"));
    REQUIRE_THAT(S(TriAssertIncludes(nullptr, Arr2(GStr("hello world"), GStr("xyz")))),
                 ContainsSubstring("hello world"));
}

TEST_CASE("triAssertExcludes - absence check", "[tri][generic-assert][triAssertExcludes]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertExcludes(nullptr, Arr2(GStr("hello world"), GStr("xyz")))) == "OK");
    REQUIRE_THAT(S(TriAssertExcludes(nullptr, Arr2(GStr("hello world"), GStr("world")))),
                 StartsWith("FAIL:expected excludes world"));
    REQUIRE_THAT(S(TriAssertExcludes(nullptr, Arr2(GStr("hello world"), GStr("world")))),
                 ContainsSubstring("hello world"));
}

TEST_CASE("triAssertMatches - alias for triAssertIncludes", "[tri][generic-assert][triAssertMatches]")
{
    InitFixture fix;
    REQUIRE(S(TriAssertMatches(nullptr, Arr2(GStr("hello world"), GStr("world")))) == "OK");
    REQUIRE_THAT(S(TriAssertMatches(nullptr, Arr2(GStr("hello world"), GStr("xyz")))),
                 StartsWith("FAIL:expected includes xyz"));
}

// ============================================================================
// Registration check
// ============================================================================

TEST_CASE("Generic assert verbs are registered in GGameState", "[tri][generic-assert][registration]")
{
    InitFixture fix;
    // Forward declarations of TriAssert etc. above force GameStateExtTestGeneric.cpp
    // into the link, which carries INIT_MODULE(GameStateExtTestGeneric,3).  The
    // module's static initializer runs at startup; InitModules() below fires it.
    GGameState.Reset();
    Poseidon::Foundation::InitModules();

    AutoArray<RStringS> functions;
    GGameState.AppendFunctionList(functions, [](const char*) { return true; });

    auto contains = [&](const char* name)
    {
        for (int i = 0; i < functions.Size(); ++i)
            if (strcmp(functions[i].Data(), name) == 0)
                return true;
        return false;
    };

    REQUIRE(contains("triAssert"));
    REQUIRE(contains("triRefute"));
    REQUIRE(contains("triAssertEq"));
    REQUIRE(contains("triAssertNe"));
    REQUIRE(contains("triAssertGt"));
    REQUIRE(contains("triAssertGe"));
    REQUIRE(contains("triAssertLt"));
    REQUIRE(contains("triAssertLe"));
    REQUIRE(contains("triAssertNear"));
    REQUIRE(contains("triAssertChanged"));
    REQUIRE(contains("triAssertIncludes"));
    REQUIRE(contains("triAssertExcludes"));
    REQUIRE(contains("triAssertMatches"));
}
