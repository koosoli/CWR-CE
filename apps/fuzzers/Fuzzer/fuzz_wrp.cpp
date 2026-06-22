// WrpReader.hpp needs AutoArray / Vector3 / Matrix4 in scope (it normally rides the
// engine PCH); pull them in explicitly since the harness gets no PCH.
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Math/Math3D.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

#include "fuzz_init.hpp"
#include "fuzz_structure.hpp"

#include <Poseidon/World/Terrain/WrpReader.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

// libFuzzer harness for the WRP terrain/world reader (WrpReader) -- custom islands
// ride inside downloaded missions / addons, so the world header (format magic,
// grid/terrain dimensions, then count-driven height / object / texture arrays) is
// attacker-controlled. WrpReader::Load(QIStream&) is a memory entry. A thrown parse
// error is handled; only a memory-safety fault reaches ASan.

using namespace Poseidon;

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // WrpReader::Load dispatches on a 4-byte magic; raw mutations almost never hit one,
    // so force a valid header. Pick OPRW (version 3) or the RVW family (64x64 grid) from
    // an input byte so both format branches stay reachable.
    std::vector<uint8_t> buf;
    if (size && (data[0] & 1))
    {
        static const uint8_t H[] = {'O', 'P', 'R', 'W', 3, 0, 0, 0}; // OPRW v3
        buf = fuzz::ForceHeader(H, sizeof(H), data, size);
    }
    else
    {
        static const uint8_t H[] = {'2', 'W', 'V', 'R', 64, 0, 0, 0, 64, 0, 0, 0}; // RVW v2, 64x64
        buf = fuzz::ForceHeader(H, sizeof(H), data, size);
    }
    try
    {
        WrpReader reader;
        QIStream f(reinterpret_cast<char*>(buf.data()), static_cast<int>(buf.size()));
        reader.Load(f);
    }
    catch (...)
    {
    }
    return 0;
}
