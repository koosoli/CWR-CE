// Lip-file lookup must honour the voice-language suffix.
//
// User report (Czech UI / Czech voice on RemasterDemo): characters in
// the demo intro scene speak the Czech voiceover correctly, but their
// lips don't move.  English / English plays correctly.  The demo's
// sound directory carries both base and Czech variants for each line:
//
//   sound/s02v_101.ogg              (English voice)
//   sound/s02v_101.lip              (English-timed phonemes)
//   sound/s02v_101.Czech.ogg        (Czech voice)
//   sound/s02v_101.Czech.lip        (Czech-timed phonemes)
//
// FindSound resolves the wave via `WithLangSuffix` when the language
// matches and the suffixed file exists, so the wave is loaded with
// its Czech audio.  But the lip lookup in ManLipInfo::AttachWave only
// derives `<wave-name>.lip` from the wave's own Name(): if the wave's
// Name() lost the language suffix anywhere along the dispatch chain
// (cutscene / script / direct path), the lookup ends up at
// `s02v_101.lip` (the English phoneme track) — or, when no base
// .lip exists, at nothing.  The voice plays in Czech with English-
// timed (or absent) lip animation.
//
// The fix is bidirectional suffix-awareness in AttachWave: when the
// derived lip path doesn't exist, try also adding `WithLangSuffix`
// (mirroring the existing WithoutLangSuffix fallback in the opposite
// direction).
//
// This file pins both directions plus the trivial baseline cases.

#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Audio/Dummy/WaveDummy.hpp>
#include <Poseidon/Audio/VoiceLangPath.hpp>
#include <Poseidon/Core/Global.hpp>
#include <Poseidon/World/Entities/Infantry/Head.hpp>
#include <Poseidon/IO/ParamFileExt.hpp>
#include <Poseidon/UI/Settings/GameSettingsConfig.hpp>
#include <Poseidon/UI/OptionsUI.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <spdlog/sinks/callback_sink.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <catch2/catch_message.hpp>
#include <initializer_list>
#include <system_error>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Time/Time.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

using Poseidon::Foundation::Time;
using namespace Poseidon;

// FindSound is declared by <Poseidon/UI/OptionsUI.hpp> (a light header).
// SetMission / SetBaseDirectory live in optionsUI.cpp; forward-declare them
// the same way dynSound.cpp / uiMapDialogs.cpp do, to avoid pulling in the
// heavy <Poseidon/UI/OptionsUICommon.hpp> umbrella.
namespace Poseidon
{
void SetBaseDirectory(RString dir);
void SetMission(RString world, RString mission, RString subdir);
} // namespace Poseidon

namespace
{

// Minimal valid .lip body: frame header + one phoneme line.  Anything
// non-empty exercises the parser past the EOF early-out so PhonemeCount
// goes >= 1 on a successful load.
constexpr const char* kLipBody = "frame = 0.11\n0.0, 0\n0.2, 5\n";

std::filesystem::path TempLipDir()
{
    auto dir = std::filesystem::temp_directory_path() / "cwr-lip-voice-suffix";
    std::filesystem::create_directories(dir);
    return dir;
}

void WriteLipFile(const std::filesystem::path& p)
{
    std::ofstream f(p);
    f << kLipBody;
}

void RemoveIfExists(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove(p, ec);
}

// RAII helper: saves + restores the live voice-language setting so each
// case starts from a known state and doesn't leak across tests.
struct ScopedVoiceLanguage
{
    std::string prev;
    explicit ScopedVoiceLanguage(const char* lang) : prev(GetSelectedVoiceLanguage())
    {
        SetSelectedVoiceLanguage(lang);
    }
    ~ScopedVoiceLanguage() { SetSelectedVoiceLanguage(prev.c_str()); }
};

// Captures spdlog payloads across all category loggers for its lifetime.
// Mirrors the helper in Core/test_logging.cpp.
struct LogSpy
{
    std::vector<std::string> messages;
    std::shared_ptr<spdlog::sinks::callback_sink_mt> sink;

    LogSpy()
    {
        sink = std::make_shared<spdlog::sinks::callback_sink_mt>(
            [this](const spdlog::details::log_msg& m) { messages.emplace_back(m.payload.data(), m.payload.size()); });
        bool any = false;
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
            if (LogDetail::g_loggers[i])
            {
                LogDetail::g_loggers[i]->sinks().push_back(sink);
                any = true;
            }
        if (!any)
            spdlog::default_logger()->sinks().push_back(sink);
    }

    ~LogSpy()
    {
        auto remove = [this](spdlog::logger* l)
        {
            auto& s = l->sinks();
            s.erase(std::remove(s.begin(), s.end(), sink), s.end());
        };
        for (int i = 0; i < static_cast<int>(Poseidon::Foundation::LogCategory::_Count); ++i)
            if (LogDetail::g_loggers[i])
                remove(LogDetail::g_loggers[i]);
        remove(spdlog::default_logger_raw());
    }

    bool Saw(const std::string& substr) const
    {
        return std::any_of(messages.begin(), messages.end(),
                           [&](const std::string& m) { return m.find(substr) != std::string::npos; });
    }
};

} // namespace

TEST_CASE("Lip lookup: baseline English voice + English lip loads", "[Audio][lip][voice-lang]")
{
    const auto dir = TempLipDir();
    const auto base = dir / "voice_baseline.lip";
    WriteLipFile(base);

    ScopedVoiceLanguage scoped("English");

    Ref<WaveDummy> wave = new WaveDummy(RString((dir / "voice_baseline.ogg").string().c_str()));
    ManLipInfo info;
    REQUIRE(info.AttachWave(wave));
    CHECK(info.PhonemeCount() >= 1);

    RemoveIfExists(base);
}

TEST_CASE("Lip lookup: Czech-suffixed wave name resolves to Czech lip", "[Audio][lip][voice-lang]")
{
    const auto dir = TempLipDir();
    const auto czech = dir / "voice_suffix.Czech.lip";
    WriteLipFile(czech);

    ScopedVoiceLanguage scoped("Czech");

    Ref<WaveDummy> wave = new WaveDummy(RString((dir / "voice_suffix.Czech.ogg").string().c_str()));
    ManLipInfo info;
    REQUIRE(info.AttachWave(wave));
    CHECK(info.PhonemeCount() >= 1);

    RemoveIfExists(czech);
}

TEST_CASE("Lip lookup: Czech-suffixed wave falls back to base lip when Czech missing", "[Audio][lip][voice-lang]")
{
    const auto dir = TempLipDir();
    const auto base = dir / "voice_fallback.lip";
    const auto czech = dir / "voice_fallback.Czech.lip";
    WriteLipFile(base);
    RemoveIfExists(czech);

    ScopedVoiceLanguage scoped("Czech");

    Ref<WaveDummy> wave = new WaveDummy(RString((dir / "voice_fallback.Czech.ogg").string().c_str()));
    ManLipInfo info;
    REQUIRE(info.AttachWave(wave));
    CHECK(info.PhonemeCount() >= 1);

    RemoveIfExists(base);
}

// Regression for the demo-intro bug.  Voice language is Czech and a
// Czech-timed lip track exists, but the wave's Name() does NOT carry
// the `.Czech.` suffix (the demo-intro dispatch shape).  AttachWave
// must also try `<base>.<voiceLang>.lip` so the Czech-timed track is
// picked up, not just the strip direction.
TEST_CASE("Lip lookup: unsuffixed wave name resolves to Czech lip when voice language is Czech",
          "[Audio][lip][voice-lang][regression]")
{
    const auto dir = TempLipDir();
    const auto base = dir / "voice_unsuffixed.lip";
    const auto czech = dir / "voice_unsuffixed.Czech.lip";
    RemoveIfExists(base); // no English lip — only Czech ships
    WriteLipFile(czech);  // Czech phoneme track

    ScopedVoiceLanguage scoped("Czech");

    // Wave name has NO language suffix — mirrors the demo intro
    // dispatch where FindSound's WithLangSuffix step did not fire (or
    // the path was constructed bypassing FindSound entirely).
    Ref<WaveDummy> wave = new WaveDummy(RString((dir / "voice_unsuffixed.ogg").string().c_str()));
    ManLipInfo info;

    REQUIRE(info.AttachWave(wave));
    CHECK(info.PhonemeCount() >= 1);

    RemoveIfExists(czech);
}

// Mirrors the actual demo dispatch shape as faithfully as the harness
// allows: the mission-directory component contains a dot (`demo.demo`),
// the resolved path is the full absolute path that FindSound builds,
// and both the suffixed and unsuffixed `.lip` files exist alongside
// the suffixed `.ogg` (so the wave has the Czech suffix in its Name()
// and the derived `<base>.<lang>.lip` must be picked up by the happy
// path, not by either fallback).
TEST_CASE("Lip lookup: demo-style suffixed wave on dotted mission dir", "[Audio][lip][voice-lang][regression]")
{
    const auto root = TempLipDir();
    const auto missionDir = root / "demo.demo" / "sound";
    std::filesystem::create_directories(missionDir);

    const auto baseLip = missionDir / "s02v_101.lip";
    const auto czechLip = missionDir / "s02v_101.Czech.lip";
    WriteLipFile(baseLip);
    WriteLipFile(czechLip);

    ScopedVoiceLanguage scoped("Czech");

    // The wave's Name() is what FindSound produces after applying
    // `WithLangSuffix`: an absolute path under a dot-containing mission
    // directory, with the language token wedged in before the
    // extension.  The `.lip` derivation must hit `s02v_101.Czech.lip`
    // (not strip to `s02v_101.lip`, not give up).
    Ref<WaveDummy> wave = new WaveDummy(RString((missionDir / "s02v_101.Czech.ogg").string().c_str()));
    ManLipInfo info;

    REQUIRE(info.AttachWave(wave));
    CHECK(info.PhonemeCount() >= 1);

    RemoveIfExists(baseLip);
    RemoveIfExists(czechLip);
}

// Drives AttachWave against real BIS-format `.lip` files copied into
// the test fixture tree.  The fixtures (`tests/fixtures/audio/voice-lang/`)
// are checked-in `.lip` content from the demo intro line s02v_101,
// renamed to a neutral stem so the test isn't coupled to a specific
// mission file.  Catches lookup bugs that the synthetic three-line
// `.lip` body in the earlier cases can't (encoding quirks, CRLF,
// real-world phoneme counts, etc.).
TEST_CASE("Lip lookup: real BIS `.lip` content resolves under Czech voice language",
          "[Audio][lip][voice-lang][regression]")
{
    namespace fs = std::filesystem;
    const fs::path fixtureDir = fs::path(TESTS_ROOT_DIR) / "fixtures" / "audio" / "voice-lang";
    const fs::path waveCzech = fixtureDir / "intro_line.Czech.ogg"; // doesn't need to exist on disk — WaveDummy stub
    const fs::path lipCzech = fixtureDir / "intro_line.Czech.lip";
    const fs::path lipEnglish = fixtureDir / "intro_line.lip";

    REQUIRE(fs::exists(lipCzech));
    REQUIRE(fs::exists(lipEnglish));

    ScopedVoiceLanguage scoped("Czech");

    Ref<WaveDummy> wave = new WaveDummy(RString(waveCzech.string().c_str()));
    ManLipInfo info;

    REQUIRE(info.AttachWave(wave));
    CHECK(info.PhonemeCount() >= 1);
}

// End-to-end coverage: a synthetic test mission (description.ext +
// sound/test_voice.{ogg,lip} pairs for English and Czech) is driven
// through the FindSound -> WaveDummy -> AttachWave dispatch chain for
// both voice-language settings.  This is the closest unit-level shape
// to the live demo intro:
//
//   1. Set up Glob.header + BaseDirectory + BaseSubdirectory so
//      GetMissionDirectory() resolves to the fixture mission.
//   2. SetMission() loads the fixture's description.ext into
//      ExtParsMission so FindSound can resolve "test_voice".
//   3. Toggle the selected voice language, call FindSound, build a
//      WaveDummy with the resolved pars.name, and confirm AttachWave
//      loads the matching .lip.
//
// Fixture layout:
//   tests/fixtures/audio/voice-lang-mission/voice_test.demo/
//     description.ext
//     sound/test_voice.ogg          + sound/test_voice.lip
//     sound/test_voice.Czech.ogg    + sound/test_voice.Czech.lip
TEST_CASE("FindSound + AttachWave: synthetic mission resolves EN and CZ lip", "[Audio][lip][voice-lang][mission]")
{
    namespace fs = std::filesystem;

    const fs::path missionsParent = fs::path(TESTS_ROOT_DIR) / "fixtures" / "audio" / "voice-lang-mission";
    const fs::path missionDir = missionsParent / "voice_test.demo";

    // Sanity: the synthetic mission must exist on disk before SetMission
    // tries to parse its description.ext.
    REQUIRE(fs::exists(missionDir / "description.ext"));
    REQUIRE(fs::exists(missionDir / "sound" / "test_voice.ogg"));
    REQUIRE(fs::exists(missionDir / "sound" / "test_voice.lip"));
    REQUIRE(fs::exists(missionDir / "sound" / "test_voice.Czech.ogg"));
    REQUIRE(fs::exists(missionDir / "sound" / "test_voice.Czech.lip"));

    // GetMissionDirectory() == BaseDirectory + BaseSubdirectory +
    // <filename> + "." + <worldname> + "/" — so pointing
    // BaseSubdirectory at the fixture's parent (with trailing
    // separator) plus filename="voice_test" / worldname="demo" yields
    // the synthetic mission dir.
    SetBaseDirectory(RString(""));
    std::strncpy(Glob.header.filename, "voice_test", sizeof(Glob.header.filename) - 1);
    std::strncpy(Glob.header.worldname, "demo", sizeof(Glob.header.worldname) - 1);
    Glob.header.filename[sizeof(Glob.header.filename) - 1] = '\0';
    Glob.header.worldname[sizeof(Glob.header.worldname) - 1] = '\0';
    Glob.header.filenameReal = "voice_test";

    const std::string subdir = missionsParent.string() + "/";
    SetMission(RString("demo"), RString("voice_test"), RString(subdir.c_str()));

    SECTION("English voice -> base .lip")
    {
        ScopedVoiceLanguage scoped("English");

        SoundPars pars;
        const ParamEntry* entry = FindSound(RString("test_voice"), pars);
        REQUIRE(entry != nullptr);

        // No `.English.ogg` ships, so WithLangSuffix's FileExist check
        // fails and pars.name keeps the un-suffixed path.
        const std::string parsName = (const char*)pars.name;
        CHECK(parsName.find(".English.") == std::string::npos);
        CHECK(parsName.find("test_voice.ogg") != std::string::npos);

        Ref<WaveDummy> wave = new WaveDummy(pars.name);
        ManLipInfo info;
        REQUIRE(info.AttachWave(wave));
        CHECK(info.PhonemeCount() >= 1);
    }

    SECTION("Czech voice -> .Czech.lip")
    {
        ScopedVoiceLanguage scoped("Czech");

        SoundPars pars;
        const ParamEntry* entry = FindSound(RString("test_voice"), pars);
        REQUIRE(entry != nullptr);

        // `test_voice.Czech.ogg` exists, so FindSound's WithLangSuffix
        // step swaps pars.name to the suffixed override.
        const std::string parsName = (const char*)pars.name;
        CHECK(parsName.find("test_voice.Czech.ogg") != std::string::npos);

        Ref<WaveDummy> wave = new WaveDummy(pars.name);
        ManLipInfo info;
        REQUIRE(info.AttachWave(wave));
        CHECK(info.PhonemeCount() >= 1);
    }

    // An unresolved sound key is reported once, at the lookup, where the logical
    // name is still in hand -- then resolves to an empty pars.name so callers
    // (SoundObject::StartSound etc.) play nothing. Previously FindSound reported
    // via WarningMessage, a no-op in the test/app frame, so a bad sound surfaced
    // only as a context-free "Empty or nullptr name not allowed" once the empty
    // name reached the file server.
    //
    // Teeth: revert FindSound's LOG_WARN to WarningMessage and Saw() goes false.
    SECTION("unknown sound key is reported at the lookup and resolves to nothing")
    {
        LogSpy spy;
        SoundPars pars;
        const ParamEntry* entry = FindSound(RString("no_such_sound_key"), pars);

        CHECK(entry == nullptr);
        CHECK(pars.name.GetLength() == 0);
        CHECK(spy.Saw("no_such_sound_key"));
    }
}

// Regression for the Czech lip "freezes at 0.4s" bug.  The root cause
// was `fastFloor` returning floor(x)-1 on x64 for positive fractional
// inputs <0.5, which made `GetPhase` overflow the frame window and
// emit lipPhase < -0.1 mid-utterance.  `Head::Animate` then cleared
// `_lipInfo`, freezing lips for the rest of the audio.
//
// This test pins the engine-side invariant: walking GetPhase across
// every offset in (0, phoneme-end] must keep lipPhase >= -0.1 (only
// the terminator at the very end may drop it negative).  Driven
// against the real BIS Czech lip data so any regression of the
// fastFloor/fastRound path surfaces immediately at the engine layer.
namespace
{
// Mock Glob.time via setter to drive GetPhase without a running engine.
// `Time(ms)` constructs a Time at `ms` milliseconds; `Glob.time =` is
// a public field (struct member), so this is safe in unit tests.
struct ScopedGlobTime
{
    Time saved;
    explicit ScopedGlobTime() : saved(Glob.time) {}
    ~ScopedGlobTime() { Glob.time = saved; }
    void SetMs(int ms) { Glob.time = Time(ms); }
};

void WalkLipTimelineAndAssertNoEarlyTermination(const std::filesystem::path& waveStubPath, const char* voiceLang,
                                                int phonemeEndMs)
{
    ScopedGlobTime guard;
    ScopedVoiceLanguage scoped(voiceLang);

    guard.SetMs(0);
    Ref<WaveDummy> wave = new WaveDummy(RString(waveStubPath.string().c_str()));
    ManLipInfo info;
    REQUIRE(info.AttachWave(wave));
    REQUIRE(info.PhonemeCount() >= 1);

    // Walk 5ms-by-5ms from t=0 up to (terminator - 1 frame).  In this
    // window the engine's invariant says lipPhase must stay >= -0.1
    // (the threshold `Head::Animate` uses to clear `_lipInfo`).  Past
    // that window the lipPhase is allowed to dip below -0.1 — that's
    // the natural end-of-utterance signal.
    const int safeEndMs = phonemeEndMs - 40; // exclude the terminator frame
    int worstNegativeMs = -1;
    float worstNegative = 0.0f;
    for (int ms = 5; ms < safeEndMs; ms += 5)
    {
        guard.SetMs(ms);
        const float lipPhase = info.GetPhase();
        if (lipPhase <= -0.1f)
        {
            if (lipPhase < worstNegative)
            {
                worstNegative = lipPhase;
                worstNegativeMs = ms;
            }
        }
    }
    INFO("Earliest threshold breach: ms=" << worstNegativeMs << " lipPhase=" << worstNegative);
    REQUIRE(worstNegativeMs == -1);
}

} // namespace

TEST_CASE("GetPhase invariant: lipPhase stays >= -0.1 across the full Czech BIS lip timeline",
          "[Audio][lip][voice-lang][regression][get-phase]")
{
    namespace fs = std::filesystem;
    const fs::path fixtureDir = fs::path(TESTS_ROOT_DIR) / "fixtures" / "audio" / "voice-lang";
    const fs::path waveStub = fixtureDir / "intro_line.Czech.ogg"; // .ogg need not exist on disk
    const fs::path lipFile = fixtureDir / "intro_line.Czech.lip";

    REQUIRE(fs::exists(lipFile));

    // The Czech intro_line .lip ends with a terminator at 2.360s, so
    // the safe window is offsets up to ~2.32s.
    WalkLipTimelineAndAssertNoEarlyTermination(waveStub, "Czech", /*phonemeEndMs=*/2360);
}

TEST_CASE("GetPhase invariant: lipPhase stays >= -0.1 across the full English BIS lip timeline",
          "[Audio][lip][voice-lang][regression][get-phase]")
{
    namespace fs = std::filesystem;
    const fs::path fixtureDir = fs::path(TESTS_ROOT_DIR) / "fixtures" / "audio" / "voice-lang";
    const fs::path waveStub = fixtureDir / "intro_line.ogg"; // .ogg need not exist on disk
    const fs::path lipFile = fixtureDir / "intro_line.lip";

    REQUIRE(fs::exists(lipFile));

    // English intro_line .lip terminator at 2.400s.
    WalkLipTimelineAndAssertNoEarlyTermination(waveStub, "English", /*phonemeEndMs=*/2400);
}

TEST_CASE("GetPhase: explicit offsets that originally produced lipPhase < -0.1 are now in range",
          "[Audio][lip][voice-lang][regression][get-phase]")
{
    // Pin the exact offsets that surfaced in the user-shared crash log:
    //   s02v_101.Czech.lip terminated at offset=0.367s, lipPhase=-0.1250, cursor=7/39
    //   s02v_102.Czech.lip terminated at offset=0.417s, lipPhase=-0.1607, cursor=9/58
    //
    // Both files are byte-identical Czech BIS data; intro_line.Czech.lip
    // is the s02v_101 line renamed.  Hammer those offsets directly so
    // a future regression of fastFloor/fastRound surfaces at this exact
    // line, not "somewhere in the timeline".
    namespace fs = std::filesystem;
    const fs::path fixtureDir = fs::path(TESTS_ROOT_DIR) / "fixtures" / "audio" / "voice-lang";
    const fs::path waveStub = fixtureDir / "intro_line.Czech.ogg";

    ScopedGlobTime guard;
    ScopedVoiceLanguage scoped("Czech");

    guard.SetMs(0);
    Ref<WaveDummy> wave = new WaveDummy(RString(waveStub.string().c_str()));
    ManLipInfo info;
    REQUIRE(info.AttachWave(wave));

    // The exact crash offset.
    guard.SetMs(367);
    {
        const float lipPhase = info.GetPhase();
        INFO("offset=0.367s lipPhase=" << lipPhase << " cursor=" << info.CurrentCursor());
        CHECK(lipPhase > -0.1f);
    }

    // Other offsets in the same risky window — every multiple-of-5ms
    // between two phoneme entries that broke the old algorithm.
    for (int ms : {320, 367, 400, 480, 520, 560, 600, 640, 720, 840, 1280, 1480, 1640, 1840, 2040, 2160})
    {
        guard.SetMs(ms);
        const float lipPhase = info.GetPhase();
        INFO("offset=" << (ms * 0.001f) << "s lipPhase=" << lipPhase << " cursor=" << info.CurrentCursor());
        CHECK(lipPhase > -0.1f);
    }
}
