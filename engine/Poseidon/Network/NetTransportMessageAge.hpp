#pragma once

#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

namespace Poseidon
{

inline float ComputeNetTransportMessageAge(unsigned64 now, unsigned64 last, const char* negativeContext,
                                           const char* overflowContext)
{
    if (now < last)
    {
        LOG_DEBUG(Network, "Negative time delta in {}: now = {:8x}{:08x}, last = {:8x}{:08x}", negativeContext,
                  (unsigned)(now >> 32), (unsigned)(now & 0xffffffff), (unsigned)(last >> 32),
                  (unsigned)(last & 0xffffffff));
        return 0.0f;
    }

    const unsigned64 delta = now - last;
    if (delta > 200000000)
    {
        RptF("Overflow in %s: now = %8x%08x, last = %8x%08x, delta = %.0f", overflowContext, (unsigned)(now >> 32),
             (unsigned)(now & 0xffffffff), (unsigned)(last >> 32), (unsigned)(last & 0xffffffff),
             static_cast<double>(1.e-6f * delta));
    }
    return 1.e-6f * static_cast<unsigned>(delta);
}

} // namespace Poseidon
