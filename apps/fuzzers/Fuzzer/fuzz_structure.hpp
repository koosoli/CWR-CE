#pragma once

// Structure-aware input shaping for the format harnesses.
//
// Most binary formats reject anything whose leading bytes aren't a valid magic /
// version, so a fuzzer mutating raw bytes spends almost all its budget bouncing at
// that gate and never reaches the decoder. Forcing a known-good header prefix onto
// the input keeps mutations in the body the parser actually decodes.
//
// Real-file seeds already carry the canonical header, so forcing it is a no-op for
// them; a mutation that corrupted the header gets it restored while the body bytes
// past it are preserved. For multi-format readers, the harness picks one magic from
// an input byte (so both formats stay reachable) and forces that one.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace fuzz
{
// Overwrite the first hdrLen bytes of the input with hdr, keep data[hdrLen..] as the
// fuzzable body. A shorter-than-header input becomes just the header.
inline std::vector<uint8_t> ForceHeader(const uint8_t* hdr, size_t hdrLen, const uint8_t* data, size_t size)
{
    std::vector<uint8_t> out(hdr, hdr + hdrLen);
    if (size > hdrLen)
    {
        out.insert(out.end(), data + hdrLen, data + size);
    }
    return out;
}
} // namespace fuzz
