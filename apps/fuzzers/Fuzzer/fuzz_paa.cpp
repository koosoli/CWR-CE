#include "fuzz_init.hpp"

#include <Poseidon/Graphics/Textures/PAADecoder.hpp>

#include <cstddef>
#include <cstdint>

// libFuzzer harness for the PAA/PAC texture decoder.
//
// Targets Poseidon::DecodePAABuffer -- the exact decode the PoseidonFormats C API
// (pf_image_load) and the engine texture path run on a .paa/.pac file. Textures
// ride inside downloaded missions / mods / PBOs, so the bytes are
// attacker-controlled: a tagged-block header (offsets, palette, mip table) then
// DXT/RLE-compressed mip data. Exceptions are swallowed (a thrown decode error is
// handled); only a memory-safety fault reaches ASan.

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    try
    {
        Poseidon::DecodePAABuffer(data, size, /*isPaa*/ true);
    }
    catch (...)
    {
    }
    return 0;
}
