#include "fuzz_init.hpp"

#include <Poseidon/Asset/Formats/P3D/MLODLoader.hpp>
#include <Poseidon/Asset/Formats/P3D/ODOLLoader.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

// libFuzzer harness for the P3D model readers (ODOL + MLOD).
//
// Targets ODOLLoader::loadFromBuffer / MLODLoader::loadFromBuffer -- the exact
// parse path the PoseidonFormats C API (pf_model_load) and the engine asset
// pipeline run on a P3D file. P3D models ride inside downloaded missions / mods /
// PBOs, so the bytes are attacker-controlled: a 4-byte magic ("ODOL"/"MLOD")
// selects the reader, then count-driven LOD / vertex / face / proxy / selection
// blocks. The loaders throw on malformed input (the C API wraps them in
// try/catch), so the harness swallows exceptions -- a thrown parse error is
// handled, not a crash; only a memory-safety fault reaches ASan.

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // No structure-aware header forcing here: libFuzzer's cmp-tracing already solves the
    // ODOL/MLOD magic, and forcing it would lock version/lodCount (which must stay fuzzed —
    // the count-driven LOD loop is where the bugs live) and pin each input to one format.
    if (size < 4)
    {
        return 0;
    }
    const char* p = reinterpret_cast<const char*>(data);
    const std::string src = "fuzz.p3d";
    try
    {
        if (std::memcmp(p, "ODOL", 4) == 0)
        {
            Poseidon::Asset::Formats::ODOLLoader::loadFromBuffer(p, static_cast<int>(size), src);
        }
        else if (std::memcmp(p, "MLOD", 4) == 0)
        {
            Poseidon::Asset::Formats::MLODLoader::loadFromBuffer(p, static_cast<int>(size), src);
        }
    }
    catch (...)
    {
    }
    return 0;
}
