#include "fuzz_init.hpp"

#include <Poseidon/IO/ParamFile/ParamFile.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

#include <cstddef>
#include <cstdint>

// libFuzzer harness for the rapified-config (ParamFile) binary reader.
//
// Targets ParamFile::Parse(QIStream&) -- the parser behind config.bin /
// RESOURCE.BIN and any rapified description.ext, reached below the file-type
// dispatch in ParamFile::Parse(const char*). Configs ride inside downloaded
// missions / mods / PBOs, so the bytes are attacker-controlled: a "\0raP" header
// then count-driven class / entry / array blocks with length-prefixed strings.
// Exceptions are swallowed (a thrown parse error is handled); only a
// memory-safety fault reaches ASan.

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        Poseidon::ParamFile pf;
        QIStream in(const_cast<char*>(reinterpret_cast<const char*>(data)), static_cast<int>(size));
        pf.Parse(in);
    }
    catch (...)
    {
    }
    return 0;
}
