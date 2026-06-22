#pragma once

namespace Poseidon
{

template <class GetPoolFn, class CleanupFn>
unsigned FreeNetTransportMemory(GetPoolFn&& getPool, CleanupFn&& cleanup)
{
    auto* pool = getPool();
    if (!pool)
    {
        return 0;
    }

    unsigned ret = pool->freeMemory();
    cleanup();
    return ret;
}

} // namespace Poseidon
