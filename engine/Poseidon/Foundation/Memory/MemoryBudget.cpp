#include <Poseidon/Foundation/Memory/MemoryBudget.hpp>

#include <algorithm>

namespace Poseidon::Foundation
{
bool ProcessMemoryBudget::Reserve(size_t size)
{
    // Pure accounting. The fast path (the only path) is a single atomic add plus a
    // peak update. We never refuse and never evict here: the engine has thousands
    // of call sites that assume `new` never fails (e.g. terrain
    // VertexTable::AddVertex -> _pos.Resize() -> memset), so returning null on a
    // ceiling would crash. Crossing a limit instead drives eviction
    // off the allocation path, once per frame, via JimboAllocator::FrameMaintenance.
    const size_t used = _used.fetch_add(size, std::memory_order_relaxed) + size;
    BumpPeak(used);
    return true;
}

MemoryLimits DeriveDefaultMemoryLimits(size_t physBytes)
{
    const size_t mb = static_cast<size_t>(1024) * 1024;
    const size_t headroom = static_cast<size_t>(kMemHeadroomMB) * mb;

    // Reserve OS/background headroom, then cap what remains. If less than ~1 GB would
    // be left to cap after the reserve, the machine is too small to bound without
    // trimming into the game's own working set — leave it unlimited. Covers the
    // unknown-RAM case (physBytes == 0) too.
    if (physBytes <= headroom + mb * 1024)
    {
        return {0, 0};
    }
    // hard = min(fixed cap, RAM minus the reserve) — so the cap dominates on a
    // normal/large machine and the OS always keeps at least `headroom`. soft = 75% of
    // the resolved hard, capped at its own ceiling. The cap, not a fraction of RAM, is
    // the anchor: a 2001-era game's working set does not scale with the machine.
    const size_t avail = physBytes - headroom;
    const size_t hard = std::min(static_cast<size_t>(kDefaultHardLimitMB) * mb, avail);
    const size_t soft = std::min(static_cast<size_t>(kDefaultSoftLimitMB) * mb, hard / 4 * 3);
    return {soft, hard};
}

} // namespace Poseidon::Foundation
