// Phase 1 regression scaffolding for the SIOF refactor.  Each test
// below probes one of the "default callback" slots on ParamFile (or
// QOFStream).  The test must FAIL if the slot is left at the no-op
// base implementation (i.e. the registration TU got dropped or SIOF
// clobbered the real impl back to the default).  That way, Phases 2-4
// of the refactor can flip the registration mechanism without losing
// the wiring guarantee — these tests are the contract.
//
// Two patterns are used:
//
// 1. **Behavioural** — call an operation whose result differs between
//    the base and derived impls.  Best signal; preferred when feasible.
//
// 2. **typeid identity** — read the slot via DefaultXyzFunctions() and
//    assert typeid(*slot) is not the base class.  Used when the
//    callback's only public method has no observable side effect we
//    can test in isolation (e.g. ClipboardFunctions::Copy → SDL).

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Evaluator/express.hpp>
#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>

#include <typeinfo>
#include <catch2/matchers/catch_matchers.hpp>

// Tag the InitClipboardFunctions free function so the test pulls in
// qstreamExt.cpp's registration before the typeid check.  In the real
// game GameApplication.cpp:1475 makes the same call.
extern void InitClipboardFunctions();

TEST_CASE("ParamFile: unterminated quoted string terminates at EOF", "[paramFile][fuzz]")
{
    InitLibraryElement();
    // fuzz_paramfile reproducer: a value string is opened but never closed before
    // EOF. Pre-fix, GetWord's quoted-string loop spun forever -- QIStream::get keeps
    // returning EOF without advancing the cursor -- hanging the parse. The loop now
    // stops at EOF. The test completing IS the regression: pre-fix it never returns.
    unsigned char repro[] = {0x22, 0x0b, 0x22, 0x22, 0x22, 0xfe, 0x03, 0x00, 0x0a};
    QIStream in(reinterpret_cast<char*>(repro), static_cast<int>(sizeof(repro)));
    ParamFile pf;
    pf.Parse(in);
    SUCCEED("parse of an unterminated quoted string returned instead of hanging");
}

TEST_CASE("ParamFile: unterminated string inside an array does not hang", "[paramFile][fuzz]")
{
    InitLibraryElement();
    // The array element parser (ParamRawArray::Parse) uses a second copy of GetWord
    // (ParamFilePrivate.inc) that had the same quoted-string EOF spin as the class
    // parser's copy. An array value string opened but never closed must terminate the
    // parse, not hang. (fuzz_paramfile timeout/RSS reproducer class.)
    char cfg[] = "x[]={\"abc"; // array whose first element is an unterminated string
    QIStream in(cfg, static_cast<int>(sizeof(cfg) - 1));
    ParamFile pf;
    pf.Parse(in);
    SUCCEED("array with an unterminated string element returned instead of hanging");
}

TEST_CASE("ParamFile evaluator slot is wired to real impl", "[paramFile][defaults][eval]")
{
    // GameStateEvaluatorFunctions registers via a Meyer's singleton
    // forced from InitParamFileEvaluator() inside InitLibraryElement().
    InitLibraryElement();
    GGameState.Init();

    ParamFile pf;

    // EvaluateFloatInternal routes through ParamFile::_defaultEvalFunctions.
    // Base returns 0 for any expression; the real impl evaluates the SQF.
    // "1+1" → 2 catches the no-op base; the assertion would also catch a
    // null slot (the lazy fallback returns the base instance).
    float result = pf.EvaluateFloatInternal("1+1");
    CHECK_THAT(result, Catch::Matchers::WithinAbs(2.0f, 0.001f));
}

TEST_CASE("ParamFile evaluator slot points to a derived type", "[paramFile][defaults][eval]")
{
    InitLibraryElement();
    EvaluatorFunctions* slot = ParamFile::DefaultEvalFunctions();
    REQUIRE(slot != nullptr);
    CHECK(typeid(*slot) != typeid(EvaluatorFunctions));
}

TEST_CASE("ParamFile CRC slot points to a derived type", "[paramFile][defaults][crc]")
{
    // paramFileExt.cpp (Poseidon.lib) registers PASumCalculatorFunctions
    // via a namespace-scope constructor; paramFile.cpp (El.lib) initialises
    // the slot via dynamic init.  MSVC link order puts Poseidon.lib *before*
    // El.lib, so the registration would otherwise run first and the El-side
    // default-init would clobber it back to the no-op base — broken until
    // Phase 2 added InitParamFileExtDefaults() and wired it into
    // test_main.cpp / Poseidon::InitDefaults().  The explicit Init runs
    // after all dynamic init and reinstates the real impl.
    CRCFunctions* slot = ParamFile::DefaultCRCFunctions();
    REQUIRE(slot != nullptr);
    CHECK(typeid(*slot) != typeid(CRCFunctions));
}

TEST_CASE("ParamFile preprocessor slot points to a derived type", "[paramFile][defaults][preproc]")
{
    Poseidon::PreprocessorFunctions* slot = ParamFile::DefaultPreprocFunctions();
    REQUIRE(slot != nullptr);
    CHECK(typeid(*slot) != typeid(Poseidon::PreprocessorFunctions));
}

TEST_CASE("ParamFile localize-string slot points to a derived type", "[paramFile][defaults][localize]")
{
    LocalizeStringFunctions* slot = ParamFile::DefaultLocalizeStringFunctions();
    REQUIRE(slot != nullptr);
    CHECK(typeid(*slot) != typeid(LocalizeStringFunctions));
}

TEST_CASE("QOFStream clipboard slot points to a derived type", "[paramFile][defaults][clipboard]")
{
    // Unlike the ParamFile slots, the clipboard slot is registered via
    // an explicit InitClipboardFunctions() call from each app's main()
    // (qstreamExt.cpp's GWindowsClipboardFunctions is the *target*, not
    // a self-registering global).  Make the same call here.
    InitClipboardFunctions();

    ClipboardFunctions* slot = QOFStream::DefaultClipboardFunctions();
    REQUIRE(slot != nullptr);
    CHECK(typeid(*slot) != typeid(ClipboardFunctions));
}

// GameState's archive/info slots already have their own behavioural
// coverage in tests/unit/engine/Poseidon/World/test_gamestate_vars_roundtrip.cpp
// (commit bbf14e14 introduced that as the canonical example).  Not
// duplicated here.
