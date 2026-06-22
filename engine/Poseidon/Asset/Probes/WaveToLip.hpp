#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Poseidon
{

struct LipRow
{
    double time;
    int    phase; // 0..7 mouth-open buckets, or -1 terminator
};

// Bucket boundaries used by the original BIS WaveToLip.
// Per-frame energy is normalized to [0..1] vs the loudest frame and mapped to
// the first bucket whose threshold the value is <=. Phase 7 is the catch-all.
constexpr double kLipFrameSeconds = 0.04; // 40 ms windows
struct LipBucket
{
    double maxNormalized;
    int    phase;
};
constexpr LipBucket kLipBuckets[] = {
    {0.05, 0}, {0.20, 1}, {0.35, 2}, {0.50, 3}, {0.65, 4}, {0.80, 5}, {0.95, 6},
};
constexpr int kLipBucketCount = sizeof(kLipBuckets) / sizeof(kLipBuckets[0]);
constexpr int kLipPhaseMax    = 7;

inline int LipBucketForEnergy(double normalized)
{
    for (int i = 0; i < kLipBucketCount; i++)
    {
        if (normalized <= kLipBuckets[i].maxNormalized)
            return kLipBuckets[i].phase;
    }
    return kLipPhaseMax;
}

std::vector<LipRow> ComputeLipRows(const int16_t* samples, size_t sampleCount, int sampleRate);
bool                WriteLipFile(const std::string& path, const std::vector<LipRow>& rows);

// Collapse interleaved 16-bit PCM to a single mono track by averaging channels
// (the lip energy pass needs one channel; the engine mixer likewise sums them).
// `channels` <= 1 returns the input unchanged. Matches what the engine plays, so
// any layout SoundLoadFile decodes (mono / stereo / N-channel) is supported.
std::vector<int16_t> DownmixToMono(const int16_t* interleaved, size_t totalSamples, int channels);

// Load audio via the engine loader (SoundLoadFile — same decode as the game) and
// run ComputeLipRows without writing a file; any channel layout is downmixed to
// mono. `outAudioSeconds` is the decoded PCM duration. Returns 0 on success or:
// 1=load failed, 2=unsupported sample depth (not 16-bit), 3=silent.
int ComputeLipRowsFromAudio(const std::string& inputPath, std::vector<LipRow>& outRows, double& outAudioSeconds);

// One-shot: load audio (any channel layout, downmixed to mono), run
// ComputeLipRows + WriteLipFile.
// Returns 0 on success or: 1=load failed, 2=unsupported sample depth, 3=silent, 4=write failed.
int GenerateLipFromAudio(const std::string& inputPath, const std::string& outputPath);

} // namespace Poseidon
