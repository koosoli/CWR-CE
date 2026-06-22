#include "fuzz_init.hpp"
#include "fuzz_structure.hpp"

#include <Poseidon/Audio/Streaming/WaveStream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// libFuzzer harness for the RIFF/WAV reader behind SoundLoadMemory (".wav" ->
// WaveLoadMemoryStream). WAV sounds ride inside downloaded missions / addons, so
// the RIFF chunk sizes and the WAVEFORMATEX are attacker-controlled. SoundLoadMemory
// is a clean memory entry and GetData drives the sample read. A thrown error is
// handled; only a memory-safety fault reaches ASan.

using namespace Poseidon;

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Force a valid "RIFF....WAVE" header so mutations reach the chunk walk +
    // WAVEFORMATEX + sample read instead of bouncing at the RIFF/WAVE fourcc check.
    static const uint8_t HDR[] = {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'};
    std::vector<uint8_t> buf = fuzz::ForceHeader(HDR, sizeof(HDR), data, size);
    try
    {
        WaveStream* ws = SoundLoadMemory(buf.data(), buf.size(), ".wav");
        if (ws)
        {
            WAVEFORMATEX fmt;
            ws->GetFormat(fmt);
            int total = ws->GetUncompressedSize();
            int cap = (total < 0) ? 0 : std::min(total, 16 << 20); // bound the decoded read
            std::vector<char> buf(4096);
            for (int off = 0; off < cap; off += static_cast<int>(buf.size()))
            {
                ws->GetData(buf.data(), off, std::min(static_cast<int>(buf.size()), cap - off));
            }
            delete ws;
        }
    }
    catch (...)
    {
    }
    return 0;
}
