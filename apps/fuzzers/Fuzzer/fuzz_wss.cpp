#include "fuzz_init.hpp"

#include <Poseidon/Audio/Streaming/WaveStream.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

// libFuzzer harness for the in-engine audio decoders behind SoundLoadMemory --
// reached for any sound that rides inside a downloaded mission / addon. The header
// (WSS magic + delta-pack flag + an attacker-controlled WAVEFORMATEX) and the
// delta-pack sample decompression are BIS-proprietary. WSS is the default path and
// .wav exercises the RIFF reader; .ogg routes to libvorbis (fuzzed upstream), so
// it is skipped. Driving GetData() over the stream triggers the decode. A thrown
// error is handled; only a memory-safety fault reaches ASan.

using namespace Poseidon;

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < 1)
        return 0;

    // Force a valid WSS header with a chosen delta-pack mode so the BIS delta
    // decompressor is actually exercised -- real seeds are uncompressed (deltaPack
    // 0), so plain mutation almost never sets it. data[0] picks 0/4/8; the rest is
    // the (attacker-controlled) WAVEFORMATEX + delta-packed audio payload.
    static const unsigned char dpModes[] = {0, 4, 8};
    std::vector<unsigned char> wss;
    wss.reserve(size + 8);
    wss.push_back('W');
    wss.push_back('S');
    wss.push_back('S');
    wss.push_back('0');
    wss.push_back(dpModes[data[0] % 3]);
    wss.push_back(0); // resvd
    wss.push_back(0);
    wss.push_back(0);
    wss.insert(wss.end(), data + 1, data + size);

    try
    {
        WaveStream* ws = SoundLoadMemory(wss.data(), wss.size(), ".wss");
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
