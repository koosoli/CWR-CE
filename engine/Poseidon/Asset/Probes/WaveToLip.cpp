// Generate BIS .lip phoneme files from PCM audio.
//
// Algorithmically 1:1 with the original tmp/snd/Snd/WaveToLip/waveToLIP.cpp:
//   - 40ms windows
//   - per-window energy = sum of |sample[i] - sample[i-1]| within the window
//   - normalize by max energy across all windows
//   - bucket via kLipBuckets thresholds → phoneme 0..7
//   - emit (time, phase) only when phase changes vs the previous frame
//   - terminator (end-time, -1) when the last frame had a non-mouth-closed phase
//
// Structured as library functions that plug into the Poseidon::Tools::InspectSound
// stack, so any format the engine can load (WAV/WSS/OGG) works as input.

#include <Poseidon/Asset/Probes/WaveToLip.hpp>

#include <Poseidon/Audio/Streaming/WaveStream.hpp>
#include <Poseidon/IO/FileServer.hpp>
#include <Poseidon/IO/FileServerMT.hpp>
#include <Poseidon/Foundation/Common/Win.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{

std::vector<LipRow> ComputeLipRows(const int16_t* samples, size_t sampleCount, int sampleRate)
{
    std::vector<LipRow> rows;
    if (!samples || sampleCount == 0 || sampleRate <= 0)
        return rows;

    const int frameSamples = static_cast<int>(kLipFrameSeconds * sampleRate);
    if (frameSamples <= 1)
        return rows;
    const int frameCount = static_cast<int>(sampleCount) / frameSamples;
    if (frameCount <= 0)
        return rows;

    std::vector<double> energies(frameCount, 0.0);
    double maxEnergy = 0.0;
    const int16_t* p = samples;
    for (int f = 0; f < frameCount; f++)
    {
        // Per-frame energy is the sum of absolute deviations from the
        // frame's first sample (NOT first-order deltas between adjacent
        // samples).  `firstSample` therefore stays fixed across the inner
        // loop — overwriting it after each iteration would silently flip
        // the algorithm to first-order differencing and produce different
        // bucket placements (and disagree with BIS `WAVToLIP.exe`).
        double sum = 0.0;
        const int firstSample = *p++;
        for (int i = 1; i < frameSamples; i++)
        {
            const int v = *p++;
            sum += std::abs(v - firstSample);
        }
        energies[f] = sum;
        if (sum > maxEnergy)
            maxEnergy = sum;
    }
    if (maxEnergy <= 0.0)
        return rows; // pure silence — no phonemes to emit

    const double invMax = 1.0 / maxEnergy;
    const double dt = static_cast<double>(frameSamples) / sampleRate;
    double time = 0.0;
    int currentPhase = -1;
    for (int f = 0; f < frameCount; f++)
    {
        const int phase = LipBucketForEnergy(energies[f] * invMax);
        if (phase != currentPhase)
        {
            rows.push_back({time, phase});
            currentPhase = phase;
        }
        time += dt;
    }
    if (currentPhase != -1)
    {
        rows.push_back({time, -1}); // terminator
    }
    return rows;
}

bool WriteLipFile(const std::string& path, const std::vector<LipRow>& rows)
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        return false;
    out << "frame = " << std::fixed;
    out.precision(3);
    out << kLipFrameSeconds << "\r\n";
    for (const auto& r : rows)
    {
        out << std::fixed;
        out.precision(3);
        out << r.time << ", ";
        out.precision(0);
        out << static_cast<double>(r.phase) << "\r\n";
    }
    return out.good();
}

std::vector<int16_t> DownmixToMono(const int16_t* interleaved, size_t totalSamples, int channels)
{
    if (!interleaved || totalSamples == 0)
        return {};
    if (channels <= 1)
        return std::vector<int16_t>(interleaved, interleaved + totalSamples);

    const size_t frames = totalSamples / static_cast<size_t>(channels);
    std::vector<int16_t> mono(frames);
    for (size_t f = 0; f < frames; f++)
    {
        int sum = 0;
        for (int c = 0; c < channels; c++)
            sum += interleaved[f * channels + c];
        mono[f] = static_cast<int16_t>(sum / channels);
    }
    return mono;
}

int ComputeLipRowsFromAudio(const std::string& inputPath, std::vector<LipRow>& outRows, double& outAudioSeconds)
{
    outRows.clear();
    outAudioSeconds = 0.0;

    if (!GFileServer)
        GFileServer = new FileServerST(0);

    Ref<WaveStream> stream = SoundLoadFile(inputPath.c_str());
    if (!stream)
        return 1;

    WAVEFORMATEX fmt{};
    stream->GetFormat(fmt);
    // SoundLoadFile decodes every supported container (OGG / WSS / WAV) to
    // interleaved 16-bit PCM — that is the engine's canonical mix format and all
    // the lip energy pass understands. Channel layout is handled below (downmix),
    // so only a non-16-bit sample depth is unsupported.
    if (fmt.wBitsPerSample != 16)
        return 2;
    const int channels = fmt.nChannels > 0 ? static_cast<int>(fmt.nChannels) : 1;

    const int total = stream->GetUncompressedSize();
    std::vector<int16_t> interleaved(total / 2);
    if (total > 0 && stream->GetData(interleaved.data(), 0, total) < total)
        return 1;

    const std::vector<int16_t> mono = DownmixToMono(interleaved.data(), interleaved.size(), channels);

    if (fmt.nSamplesPerSec > 0)
        outAudioSeconds = static_cast<double>(mono.size()) / static_cast<double>(fmt.nSamplesPerSec);

    outRows = ComputeLipRows(mono.data(), mono.size(), static_cast<int>(fmt.nSamplesPerSec));
    if (outRows.empty())
        return 3;

    return 0;
}

int GenerateLipFromAudio(const std::string& inputPath, const std::string& outputPath)
{
    std::vector<LipRow> rows;
    double seconds = 0.0;
    const int rc = ComputeLipRowsFromAudio(inputPath, rows, seconds);
    if (rc != 0)
        return rc;

    if (!WriteLipFile(outputPath, rows))
        return 4;

    return 0;
}

} // namespace Poseidon
