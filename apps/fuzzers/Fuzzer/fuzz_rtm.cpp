#include "fuzz_structure.hpp"

#include <Poseidon/Asset/Formats/RTM/RTMReader.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

// libFuzzer harness for the RTM (keyframe animation) reader.
//
// Target: Poseidon::Asset::Formats::readRTMFromMemory — the exact parser behind
// the PoseidonFormats C API (pf_rtm_load_from_memory) and the asset-probe path
// (AssetInfo.cpp). RTM files ride inside downloaded missions / mods / PBOs, so
// the reader sees attacker-controlled bytes (magic, bone/phase counts, then
// count-driven keyframe blocks). The parser is header-only and std-only
// (std::istringstream), so no engine init is required; ASan instruments the
// inline code in this translation unit.
//
// Build with a fuzz preset so Release/NDEBUG, ASan, and SanitizerCoverage are all active.
// NDEBUG matches the shipping tools/Blender-addon DLL, so any count or read that
// is unchecked there is unchecked here too.

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Force the 8-byte "RTM_0100" magic so mutations reach the step/bone-count header
    // and the count-driven bone-name + keyframe blocks instead of failing the strcmp.
    static const uint8_t HDR[] = {'R', 'T', 'M', '_', '0', '1', '0', '0'};
    std::vector<uint8_t> buf = fuzz::ForceHeader(HDR, sizeof(HDR), data, size);
    Poseidon::Asset::Formats::RTMAnimation anim;
    Poseidon::Asset::Formats::readRTMFromMemory(buf.data(), buf.size(), anim);
    return 0;
}
