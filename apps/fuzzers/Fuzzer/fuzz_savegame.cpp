#include "fuzz_init.hpp"
#include "fuzz_structure.hpp"

#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

// libFuzzer harness for the binary save-game / SerializeBin deserializer.
//
// A saved game (.fps) — and any SerializeBin archive — rides untrusted: it can be
// hand-edited or shared. ParamFile::SerializeBin reads the "\0raP" magic + a version,
// then ParamClass::SerializeBin walks a count-driven tree of class / value / array
// entries through SerializeBinStream's LoadInt / LoadChar / length-prefixed string
// reads and the Transfer*Array primitives (count read off the wire → Realloc).
//
// Distinct from fuzz_paramfile, which drives the rapified Parse(QIStream&) decoder:
// this exercises the SerializeBinStream path the WRP fuzzer only partially reached.
// A thrown parse error is handled; only a memory-safety fault reaches ASan.

using namespace Poseidon;

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Force a valid "\0raP" magic + version 4 so mutations reach the entry tree
    // instead of failing the magic / EBadVersion gate.
    static const uint8_t HDR[] = {0x00, 0x72, 0x61, 0x50, 0x04, 0x00, 0x00, 0x00};
    std::vector<uint8_t> buf = fuzz::ForceHeader(HDR, sizeof(HDR), data, size);
    try
    {
        ParamFile pf;
        QIStream in(reinterpret_cast<char*>(buf.data()), static_cast<int>(buf.size()));
        SerializeBinStream f(&in);
        pf.SerializeBin(f);
    }
    catch (...)
    {
    }
    return 0;
}
