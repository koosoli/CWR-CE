#include "SoundCommand.hpp"
#include <Poseidon/Audio/Streaming/WaveStream.hpp>
#include <Poseidon/Audio/VoiceLangPath.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/IO/FileServer.hpp>
#include <Poseidon/IO/FileServerMT.hpp>
#include <Poseidon/Asset/Probes/AssetInfo.hpp>
#include <Poseidon/Asset/Probes/SoundPlayer.hpp>
#include <Poseidon/Asset/Probes/WaveToLip.hpp>
#include <PoseidonOpenAL/EFXPresets.hpp>
#include <cjson/cJSON.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <string.h>
#include <CLI/App.hpp>
#include <CLI/Error.hpp>
#include <CLI/Option.hpp>
#include <CLI/Validators.hpp>
#include <cstdlib>
#include <format>
#include <functional>
#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace PoseidonTools
{

namespace
{

static std::string FixturePath()
{
    const char* candidates[] = {
        "apps/tools/Tools/fixtures/check.wav",
        "fixtures/check.wav",
    };
    for (const char* p : candidates)
    {
        std::ifstream f(p);
        if (f.good())
            return p;
    }
    return "";
}

struct PlayOptions
{
    bool eax = false;
    std::string efxPreset;
    float size = 50.0f;
};

static bool PlaySoundFile(const char* path, bool silent, const PlayOptions& opts = {})
{
    if (!GFileServer)
        GFileServer = new FileServerST(0);

    Poseidon::SoundPlayer player;
    if (!player.isReady())
    {
        std::cerr << "Error: Failed to initialize audio system\n";
        return false;
    }

    if (opts.eax || !opts.efxPreset.empty())
    {
        if (!player.enableEAX(true))
        {
            std::cerr << "Warning: EAX/EFX not available on this audio backend\n";
        }
        else if (!opts.efxPreset.empty())
        {
            if (!player.applyEFXByName(opts.efxPreset.c_str(), opts.size))
            {
                std::cerr << "Error: Unknown EFX preset '" << opts.efxPreset << "'\n";
                std::cerr << "Use 'sound efx-list' to see available presets.\n";
                return false;
            }
            if (!silent)
                std::cout << "EFX: " << opts.efxPreset << " (size=" << opts.size << ")\n";
        }
        else
        {
            // --eax without --efx: use game default (plain, size=50)
            player.applyEFXByName("plain", opts.size);
            if (!silent)
                std::cout << "EAX: enabled (plain, size=" << opts.size << ")\n";
        }
    }

    if (!player.play(path))
    {
        std::cerr << "Error: Failed to load: " << path << "\n";
        return false;
    }

    if (!silent)
        std::cout << "Playing '" << path << "' (" << player.duration() << "s)...\n";

    float dur = player.duration();
    if (dur > 0.0f)
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(dur * 1000) + 50));

    return true;
}
static void setupSoundInspect(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("inspect", "Inspect audio file metadata");
    static std::string inputPath;
    cmd->add_option("input", inputPath, "Input audio file path (WAV/WSS/OGG)")->required()->check(CLI::ExistingFile);

    cmd->callback(
        []()
        {
            auto info = Poseidon::InspectSound(inputPath);
            if (!info.valid)
            {
                std::cerr << "Error: Failed to load: " << inputPath << std::endl;
                throw CLI::RuntimeError(1);
            }

            std::cout << "File: " << inputPath << std::endl;
            std::cout << "Format: " << info.format << std::endl;
            std::cout << "Channels: " << info.channels << std::endl;
            std::cout << "Sample Rate: " << info.sampleRate << " Hz" << std::endl;
            std::cout << "Bit Depth: " << info.bitDepth << std::endl;

            int totalMs = static_cast<int>(info.duration * 1000.0);
            int secs = totalMs / 1000;
            int ms = totalMs % 1000;
            std::cout << "Duration: " << secs << "." << (ms / 100) << (ms / 10 % 10) << (ms % 10) << "s" << std::endl;

            std::cout << "Size: " << info.uncompressedSize << " bytes (uncompressed)" << std::endl;

            if (info.fileSize > 0)
            {
                std::cout << "File Size: " << info.fileSize << " bytes" << std::endl;
                if (info.format == "WSS" && info.compressionRatio > 0.0)
                {
                    std::cout << "Compression: " << std::fixed;
                    std::cout.precision(1);
                    std::cout << (info.compressionRatio * 100.0) << "%" << std::endl;
                }
            }
        });
}
static void setupSoundLip(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("lip", "Generate .lip lip-sync file from a 16-bit mono audio file");
    static std::string inputPath;
    static std::string outputPath;
    cmd->add_option("input", inputPath, "Input audio (WAV/WSS/OGG, 16-bit mono)")->required()->check(CLI::ExistingFile);
    cmd->add_option("-o,--output", outputPath, "Output .lip path (default: input with .lip extension)");

    cmd->callback(
        []()
        {
            std::string out = outputPath;
            if (out.empty())
            {
                const auto dot = inputPath.find_last_of('.');
                out = (dot == std::string::npos ? inputPath : inputPath.substr(0, dot)) + ".lip";
            }
            const int rc = Poseidon::GenerateLipFromAudio(inputPath, out);
            switch (rc)
            {
                case 0:
                    std::cout << inputPath << " -> " << out << std::endl;
                    return;
                case 1:
                    std::cerr << "Error: cannot read audio: " << inputPath << std::endl;
                    break;
                case 2:
                    std::cerr << "Error: " << inputPath << " is not 16-bit mono" << std::endl;
                    break;
                case 3:
                    std::cerr << "Error: " << inputPath << " is silent (no signal)" << std::endl;
                    break;
                case 4:
                    std::cerr << "Error: cannot write " << out << std::endl;
                    break;
                default:
                    std::cerr << "Error: unknown failure (code " << rc << ")" << std::endl;
            }
            throw CLI::RuntimeError(rc);
        });
}
struct LipFrame
{
    double time;
    int phase; // 0..7, or -1 for terminator
};

// Resolve a .lip path for the audio at `audioPath`, honouring an optional
// voice-language override (so `lipsim s02v_101.Czech.ogg --lang Czech`
// behaves like the engine's runtime fallback chain).  Returns empty on
// failure.
static std::string ResolveLipPath(const std::string& audioPath, const std::string& voiceLang)
{
    namespace fs = std::filesystem;
    fs::path p(audioPath);
    p.replace_extension(".lip");
    if (fs::exists(p))
        return p.string();
    if (!voiceLang.empty())
    {
        const RString suffixed = Poseidon::WithLangSuffix(RString(p.string().c_str()), RString(voiceLang.c_str()));
        if (suffixed.GetLength() > 0 && fs::exists((const char*)suffixed))
            return std::string((const char*)suffixed);
        const RString stripped = Poseidon::WithoutLangSuffix(RString(p.string().c_str()), RString(voiceLang.c_str()));
        if (stripped.GetLength() > 0 && fs::exists((const char*)stripped))
            return std::string((const char*)stripped);
    }
    return {};
}

static bool ParseLipFile(const std::string& path, double& frameSeconds, std::vector<LipFrame>& frames)
{
    std::ifstream in(path);
    if (!in)
        return false;
    frameSeconds = 0.04;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            continue;
        double frameHdr = 0;
        if (std::sscanf(line.c_str(), "frame = %lf", &frameHdr) == 1)
        {
            frameSeconds = frameHdr;
            continue;
        }
        double t = 0;
        int phase = 0;
        if (std::sscanf(line.c_str(), "%lf, %d", &t, &phase) == 2)
            frames.push_back({t, phase});
    }
    return !frames.empty();
}

// Same interpolation `ManLipInfo::GetPhase()` uses in the engine — pull the
// "current" and "next" frame for the supplied offset and linearly blend
// across the frame window.  Returns -1 once the timeline is exhausted (the
// engine clears `_lipInfo` at that point).
static double ComputePhase(const std::vector<LipFrame>& frames, double frameSeconds, double offset)
{
    if (frames.empty())
        return -1.0;
    const double invFrame = 1.0 / frameSeconds;
    const double floor = std::floor(offset * invFrame) * frameSeconds;

    size_t current = 0;
    while (current < frames.size() && frames[current].time < floor)
        ++current;
    if (current > frames.size())
        return -1.0;

    double oldPhase = 0;
    if (current > 0)
        oldPhase = frames[current - 1].phase;

    size_t c = current;
    while (c < frames.size() && frames[c].time < floor + frameSeconds)
        ++c;

    double newPhase = 0;
    if (c > 0 && c < frames.size())
        newPhase = frames[c - 1].phase;

    const double dif = offset - floor;
    return (1.0 / 7) * invFrame * (dif * newPhase + (frameSeconds - dif) * oldPhase);
}

// Mouth rendering: each visible "phase tier" gets a distinct two-char shape
// so the user can read the animation as it streams by.
static const char* MouthForPhase(int phase)
{
    switch (phase)
    {
        case -1:
            return "--"; // terminator / silent
        case 0:
            return "__"; // closed
        case 1:
            return "..";
        case 2:
            return "oo";
        case 3:
            return "oO";
        case 4:
            return "Oo";
        case 5:
            return "OO";
        case 6:
            return "Ow";
        case 7:
            return "ww"; // wide open
        default:
            return "??";
    }
}

static std::string Bar(double lipPhase, int width = 12)
{
    const double clamped = std::max(0.0, std::min(1.0, lipPhase));
    const int filled = static_cast<int>(std::round(clamped * width));
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < filled ? '#' : ' ');
    bar += "]";
    return bar;
}

static void setupSoundLipSim(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("lipsim", "Play audio with synced ASCII lip-sync visualization");
    static std::string audioPath;
    static std::string lipPath;
    static std::string voiceLang;
    static int tickMs = 40; // default = engine frame size
    static bool noPlay = false;

    cmd->add_option("audio", audioPath, "Audio file (.ogg/.wav/.wss)")->required()->check(CLI::ExistingFile);
    cmd->add_option("--lip", lipPath, "Explicit .lip path (default: derived from audio extension)");
    cmd->add_option("--lang", voiceLang, "Voice language for suffix fallback (e.g. Czech, French)");
    cmd->add_option("--tick", tickMs, "Tick interval in ms for the display (default 40)")->check(CLI::Range(10, 500));
    cmd->add_flag("--no-play", noPlay, "Skip audio playback — just dump the lip timeline");

    cmd->callback(
        []()
        {
            const std::string resolvedLip = lipPath.empty() ? ResolveLipPath(audioPath, voiceLang) : lipPath;
            if (resolvedLip.empty())
            {
                std::cerr << "Error: no .lip file found for '" << audioPath << "'";
                if (!voiceLang.empty())
                    std::cerr << " (tried derived, +" << voiceLang << " suffix, -" << voiceLang << " suffix)";
                std::cerr << "\n";
                throw CLI::RuntimeError(1);
            }

            double frameSeconds = 0.04;
            std::vector<LipFrame> frames;
            if (!ParseLipFile(resolvedLip, frameSeconds, frames))
            {
                std::cerr << "Error: could not parse .lip file: " << resolvedLip << "\n";
                throw CLI::RuntimeError(1);
            }

            std::cout << "Audio:    " << audioPath << "\n";
            std::cout << "Lip:      " << resolvedLip << "\n";
            std::cout << "Frame:    " << frameSeconds << "s\n";
            std::cout << "Phonemes: " << frames.size() << "\n";
            if (!voiceLang.empty())
                std::cout << "VoiceLang: " << voiceLang << "\n";
            const double lipEnd = frames.empty() ? 0.0 : frames.back().time;
            std::cout << "Duration: " << lipEnd << "s (per .lip terminator)\n";
            std::cout << "\n";
            std::cout << " time      phase           bar                          mouth\n";
            std::cout << " --------  --------------  ---------------------------  -----\n";

            // SoundLoadFile dereferences GFileServer during player playback.
            if (!GFileServer)
                GFileServer = new FileServerST(0);

            Poseidon::SoundPlayer player;
            const bool play = !noPlay && player.isReady();
            float audioDuration = static_cast<float>(lipEnd);
            if (play)
            {
                if (!player.play(audioPath.c_str()))
                {
                    std::cerr << "Warning: audio playback failed, continuing with lip-only timeline\n";
                }
                else
                {
                    audioDuration = std::max(audioDuration, player.duration());
                }
            }

            const auto start = std::chrono::steady_clock::now();
            const double dt = tickMs * 0.001;
            const double endTime = std::max(static_cast<double>(audioDuration), lipEnd) + 0.1;
            while (true)
            {
                const auto now = std::chrono::steady_clock::now();
                const double offset = std::chrono::duration<double>(now - start).count();
                if (offset > endTime)
                    break;

                const double lipPhase = ComputePhase(frames, frameSeconds, offset);
                // Pull the nearest phoneme entry too — the integer phase the
                // engine *would* be sampling at this offset — so the line
                // shows both the raw phoneme and the interpolated value.
                int rawPhase = -1;
                for (const auto& f : frames)
                {
                    if (f.time > offset)
                        break;
                    rawPhase = f.phase;
                }

                std::printf(" %6.3fs   raw=%-2d  cur=%5.3f  %s  %s\n", offset, rawPhase, lipPhase,
                            Bar(lipPhase).c_str(), MouthForPhase(rawPhase));
                std::fflush(stdout);

                std::this_thread::sleep_until(
                    start + std::chrono::milliseconds(static_cast<long long>((offset + dt) * 1000.0)));
            }
        });
}
static void setupSoundPlay(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("play", "Play audio file with optional EFX reverb");
    static std::string inputPath;
    static PlayOptions opts;

    cmd->add_option("input", inputPath, "Path to sound file (defaults to bundled fixture)");
    cmd->add_flag("--eax", opts.eax, "Enable EAX reverb (default preset: plain)");
    cmd->add_option("--efx", opts.efxPreset, "Apply named EFX preset (implies --eax). Use 'sound efx-list' for names");
    cmd->add_option("--size", opts.size, "Environment size for reverb scaling [2-100] (default: 50)")
        ->check(CLI::Range(2.0, 100.0));

    cmd->callback(
        []()
        {
            std::string file = inputPath;
            if (file.empty())
            {
                file = FixturePath();
                if (file.empty())
                {
                    std::cerr << "Error: No file specified. Usage: sound play <file.wav|.ogg|.wss>\n";
                    throw CLI::RuntimeError(1);
                }
            }

            if (!PlaySoundFile(file.c_str(), /*silent=*/false, opts))
                throw CLI::RuntimeError(1);

            std::cout << "Done.\n";
        });
}
static void setupSoundEFXList(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("efx-list", "List game EFX reverb presets");

    cmd->callback(
        []()
        {
            // Game presets only (matching engine SoundEnvironmentType mapping)
            struct GamePreset
            {
                const char* name;
                const char* env;
            };
            const GamePreset presets[] = {
                {"plain", "SEPlain"},         {"city", "SECity / SEForest (intentional DX8 parity)"},
                {"mountains", "SEMountains"}, {"room", "SERoom"},
                {"generic", "fallback"},
            };

            std::cout << "Game EFX presets (from engine SoundEnvironmentType):\n\n";
            for (const auto& gp : presets)
            {
                auto* entry = EFX::FindPreset(gp.name);
                if (!entry)
                    continue;
                std::cout << "  " << gp.name;
                size_t len = strlen(gp.name);
                for (size_t j = len; j < 14; ++j)
                    std::cout << ' ';
                std::cout << "size=" << entry->defaultSize << "  ← " << gp.env << "\n";
            }
            std::cout << "\nUsage: sound play <file> --efx <preset> [--size <2-100>]\n";
            std::cout << "       sound play <file> --eax  (shorthand for --efx plain)\n";
        });
}
static void setupSoundCheck(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("check", "Automated audio smoke test (CI-safe)");

    cmd->callback(
        []()
        {
            std::string fixture = FixturePath();
            if (fixture.empty())
            {
                std::cerr << "FAIL — fixture not found (run from repo root)\n";
                throw CLI::RuntimeError(1);
            }

            if (!PlaySoundFile(fixture.c_str(), /*silent=*/true))
            {
                std::cerr << "FAIL — audio playback error\n";
                throw CLI::RuntimeError(1);
            }
            std::cout << "OK — audio playback succeeded\n";
        });
}

} // namespace
// Expand sparse (time, phase) rows into a per-frame phase track [0..7] sampled
// at `frameSeconds`.  Phase -1 (terminator) and pre-roll read as 0 (mouth shut).
static std::vector<double> DenseLipPhases(const std::vector<LipFrame>& frames, double frameSeconds, int nFrames)
{
    std::vector<double> out(static_cast<size_t>(std::max(nFrames, 0)), 0.0);
    size_t idx = 0;
    double cur = 0.0;
    for (int i = 0; i < nFrames; i++)
    {
        const double t = i * frameSeconds;
        while (idx < frames.size() && frames[idx].time <= t + 1e-9)
        {
            cur = frames[idx].phase < 0 ? 0.0 : static_cast<double>(frames[idx].phase);
            ++idx;
        }
        out[static_cast<size_t>(i)] = cur;
    }
    return out;
}

// Pearson correlation over [0, n).  Both flat+equal -> 1; exactly one flat ->
// 0 (cannot confirm a match); otherwise r in [-1, 1].
static double PearsonCorr(const std::vector<double>& a, const std::vector<double>& b, int n)
{
    if (n <= 1)
        return 0.0;
    double sa = 0, sb = 0;
    for (int i = 0; i < n; i++)
    {
        sa += a[i];
        sb += b[i];
    }
    const double ma = sa / n, mb = sb / n;
    double num = 0, da = 0, db = 0;
    for (int i = 0; i < n; i++)
    {
        const double xa = a[i] - ma, xb = b[i] - mb;
        num += xa * xb;
        da += xa * xa;
        db += xb * xb;
    }
    if (da <= 1e-9 && db <= 1e-9)
        return std::abs(ma - mb) < 0.5 ? 1.0 : 0.0;
    if (da <= 1e-9 || db <= 1e-9)
        return 0.0;
    return num / std::sqrt(da * db);
}

static void setupSoundLipCheck(CLI::App& sound)
{
    auto* cmd = sound.add_subcommand("lipcheck", "Grade a .lip against its audio (MATCH / DIFFERENT_SAME_LENGTH / "
                                                 "LENGTH_MISMATCH / TOTALLY_DIFFERENT)");
    static std::string audioPath;
    static std::string lipPath;
    static std::string voiceLang;
    static bool asJson = false;
    static double durTolOpt = -1.0;
    static double corrMin = 0.6;
    cmd->add_option("audio", audioPath, "Audio file (.ogg/.wav/.wss, 16-bit mono)")
        ->required()
        ->check(CLI::ExistingFile);
    cmd->add_option("--lip", lipPath, "Explicit .lip path (default: audio path with .lip extension)");
    cmd->add_option("--lang", voiceLang, "Voice language for suffix fallback (e.g. Czech, French)");
    cmd->add_flag("--json", asJson, "Emit a single-line JSON verdict");
    cmd->add_option("--dur-tol", durTolOpt, "Duration tolerance in seconds (default: max(0.08, 5% of audio))");
    cmd->add_option("--corr-min", corrMin, "Min envelope correlation counted as a match (default 0.6)")
        ->check(CLI::Range(-1.0, 1.0));

    cmd->callback(
        []()
        {
            const std::string resolvedLip = lipPath.empty() ? ResolveLipPath(audioPath, voiceLang) : lipPath;

            std::vector<Poseidon::LipRow> regenRows;
            double audioSeconds = 0.0;
            const int arc = Poseidon::ComputeLipRowsFromAudio(audioPath, regenRows, audioSeconds);

            auto emit =
                [&](const char* verdict, int code, double lipDur, double corr, double durDiff, const char* detail)
            {
                if (asJson)
                {
                    cJSON* obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(obj, "verdict", verdict);
                    cJSON_AddStringToObject(obj, "audio", audioPath.c_str());
                    cJSON_AddStringToObject(obj, "lip", resolvedLip.c_str());
                    cJSON_AddNumberToObject(obj, "audioSeconds", audioSeconds);
                    cJSON_AddNumberToObject(obj, "lipSeconds", lipDur);
                    cJSON_AddNumberToObject(obj, "durDiff", durDiff);
                    cJSON_AddNumberToObject(obj, "corr", corr);
                    char* json = cJSON_PrintUnformatted(obj);
                    std::cout << (json ? json : "{}") << "\n";
                    cJSON_free(json);
                    cJSON_Delete(obj);
                }
                else
                {
                    std::cout << verdict << "  audio=" << audioSeconds << "s lip=" << lipDur << "s durDiff=" << durDiff
                              << "s corr=" << corr;
                    if (detail && detail[0])
                        std::cout << "  (" << detail << ")";
                    std::cout << "\n";
                }
                std::exit(code);
            };

            if (arc != 0)
                emit("ERROR", 4, 0.0, 0.0, 0.0,
                     arc == 1 ? "cannot read audio" : (arc == 2 ? "audio not 16-bit mono" : "audio silent"));
            if (resolvedLip.empty())
                emit("MISSING_LIP", 4, 0.0, 0.0, 0.0, "no .lip found for audio");

            double frameSeconds = Poseidon::kLipFrameSeconds;
            std::vector<LipFrame> lipFrames;
            if (!ParseLipFile(resolvedLip, frameSeconds, lipFrames))
                emit("ERROR", 4, 0.0, 0.0, 0.0, "cannot parse .lip");

            const double lipDur = lipFrames.empty() ? 0.0 : lipFrames.back().time;

            std::vector<LipFrame> regenFrames;
            regenFrames.reserve(regenRows.size());
            for (const auto& r : regenRows)
                regenFrames.push_back({r.time, r.phase});

            const int nLip = static_cast<int>(std::lround(lipDur / Poseidon::kLipFrameSeconds));
            const int nAudio = static_cast<int>(std::lround(audioSeconds / Poseidon::kLipFrameSeconds));
            const int overlap = std::max(0, std::min(nLip, nAudio));
            const auto denseLip = DenseLipPhases(lipFrames, frameSeconds, overlap);
            const auto denseAudio = DenseLipPhases(regenFrames, Poseidon::kLipFrameSeconds, overlap);
            const double corr = PearsonCorr(denseLip, denseAudio, overlap);

            const double durDiff = std::abs(lipDur - audioSeconds);
            const double durTol = durTolOpt > 0 ? durTolOpt : std::max(0.08, 0.05 * audioSeconds);
            const bool durMatch = durDiff <= durTol;
            const bool corrGood = corr >= corrMin;

            if (durMatch && corrGood)
                emit("MATCH", 0, lipDur, corr, durDiff, "");
            else if (durMatch && !corrGood)
                emit("DIFFERENT_SAME_LENGTH", 2, lipDur, corr, durDiff, "duration matches but the envelope does not");
            else if (corrGood)
                emit("LENGTH_MISMATCH", 1, lipDur, corr, durDiff, "envelope correlates but durations differ");
            else
                emit("TOTALLY_DIFFERENT", 3, lipDur, corr, durDiff, "different length and envelope");
        });
}

void SoundCommand::Setup(CLI::App& app)
{
    auto* sound = app.add_subcommand("sound", "Audio file operations (WAV/WSS/OGG)");
    sound->require_subcommand(1);

    setupSoundInspect(*sound);
    setupSoundLip(*sound);
    setupSoundLipCheck(*sound);
    setupSoundLipSim(*sound);
    setupSoundPlay(*sound);
    setupSoundEFXList(*sound);
    setupSoundCheck(*sound);
}

} // namespace PoseidonTools
