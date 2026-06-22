#ifndef POSEIDON_MEMORY_BUDGET_HPP
#define POSEIDON_MEMORY_BUDGET_HPP

// Process-wide allocation budget. Pure accounting + threshold policy; owns no
// memory itself. Embedded in JimboAllocator so the global heap can enforce a
// coarse ceiling and surface a soft-pressure signal — without sharding per-system
// caps into the raw allocator. Budgets that know which assets are safe to evict
// live at the subsystem edges (the FreeOnDemand registry), not here.
//
// Both limits default to 0 = unlimited, so an unconfigured engine behaves exactly
// as before.

#include <atomic>
#include <cstddef>

namespace Poseidon::Foundation
{
class ProcessMemoryBudget
{
  public:
    ProcessMemoryBudget() = default;

    void SetLimits(size_t softBytes, size_t hardBytes)
    {
        _soft.store(softBytes, std::memory_order_relaxed);
        _hard.store(hardBytes, std::memory_order_relaxed);
    }

    // Reserve `size` bytes against the budget BEFORE the backing allocation.
    // Pure accounting: advances `_used` and tracks the peak, then always admits
    // (returns true). It never refuses and never evicts — a refusal would null an
    // unchecked engine `new` and crash. Eviction in response to
    // crossing a limit is driven separately, off the allocation path, by the
    // allocator's once-per-frame FrameMaintenance().
    bool Reserve(size_t size);

    // Reconcile the requested estimate with the true backing size after a
    // successful allocation (mimalloc usable_size differs from the request).
    void Reconcile(size_t reserved, size_t actual)
    {
        if (actual > reserved)
        {
            BumpPeak(_used.fetch_add(actual - reserved, std::memory_order_relaxed) + (actual - reserved));
        }
        else if (reserved > actual)
        {
            _used.fetch_sub(reserved - actual, std::memory_order_relaxed);
        }
    }

    void Release(size_t size)
    {
        // Saturate at zero — Reconcile/Reserve estimate against the request, so a
        // pathological mismatch must never underflow the unsigned counter.
        size_t prev = _used.load(std::memory_order_relaxed);
        size_t next;
        do
        {
            next = prev >= size ? prev - size : 0;
        } while (!_used.compare_exchange_weak(prev, next, std::memory_order_relaxed));
    }

    size_t Used() const { return _used.load(std::memory_order_relaxed); }
    size_t Peak() const { return _peak.load(std::memory_order_relaxed); }
    size_t SoftLimit() const { return _soft.load(std::memory_order_relaxed); }
    size_t HardLimit() const { return _hard.load(std::memory_order_relaxed); }

    bool OverSoftLimit() const
    {
        const size_t s = SoftLimit();
        return s != 0 && Used() > s;
    }

    void ResetPeak() { _peak.store(Used(), std::memory_order_relaxed); }

  private:
    void BumpPeak(size_t used)
    {
        size_t prev = _peak.load(std::memory_order_relaxed);
        while (used > prev && !_peak.compare_exchange_weak(prev, used, std::memory_order_relaxed))
        {
            // prev reloaded by compare_exchange_weak
        }
    }

    std::atomic<size_t> _used{0};
    std::atomic<size_t> _peak{0};
    std::atomic<size_t> _soft{0};
    std::atomic<size_t> _hard{0};
};

struct MemoryLimits
{
    size_t soft = 0;
    size_t hard = 0;
};

// Absolute caps for the auto-derived backstop (MB). A CWA-era game's working set
// does NOT scale with the machine — it peaks well under 1 GB even in a 230-unit,
// 5 km-view-distance stress mission (measured ~0.6 GB process working set). So the
// backstop is anchored on a fixed ceiling, not a fraction of RAM: 80% of a 64 GB box
// would be a useless ~50 GB, far above any leak worth catching. These are generous
// (>6x the measured peak) so they never thrash real play, yet small enough that a
// runaway leak is caught long before it exhausts the machine. Override with --maxmem.
inline constexpr int kDefaultSoftLimitMB = 4096; // 4 GB
inline constexpr int kDefaultHardLimitMB = 8192; // 8 GB

// OS/background headroom reserved out of physical RAM before the cap binds. Modern
// guidance is to leave a few GB free rather than size a limit to total RAM, so the
// backstop trips before the *system* starves, not just the game. On a 16 GB+ box the
// 8 GB cap already leaves far more than this; the reserve only matters on a small
// machine, where it lowers the cap (an 8 GB box caps at 6 GB, leaving the OS its 2).
inline constexpr int kMemHeadroomMB = 2048; // 2 GB

// Default process limits used when the config leaves them on `auto`. A near-OOM
// backstop, NOT a working cap: it reserves OS headroom out of physical RAM, then
// caps the remainder at the fixed `kDefault*LimitMB` ceilings (soft = 75% of the
// resolved hard). The fixed cap dominates on a normal/large machine; on a small one
// the reserve lowers it so the OS always keeps `kMemHeadroomMB`. Compared against the
// GAME PROCESS's own residency, so on any normal machine the budget stays inert
// (FrameMaintenance returns immediately) and engages only when the process balloons
// toward the cap, where trimming caches beats an OS OOM. A machine too small to cap
// without trimming into the game's own working set (~0.6 GB measured) — and the
// unknown-RAM case `physBytes == 0` — yields {0, 0} = unlimited.
MemoryLimits DeriveDefaultMemoryLimits(size_t physBytes);

} // namespace Poseidon::Foundation

#endif // POSEIDON_MEMORY_BUDGET_HPP
