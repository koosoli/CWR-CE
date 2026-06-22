#ifndef POSEIDON_MEMORY_JIMBO_ALLOCATOR_HPP
#define POSEIDON_MEMORY_JIMBO_ALLOCATOR_HPP

// MemFunctions implementation backed by mimalloc: 16-byte-aligned, thread-safe
// allocations, with FreeOnDemand cache eviction and a process-wide budget.

#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>
#include <Poseidon/Foundation/Memory/MemoryBudget.hpp>
#include <atomic>
#include <cstddef>
#include <mimalloc.h>

namespace Poseidon::Foundation
{
// Single-allocation sanity reject. A request this large is virtually always a
// corrupted/overflowed size computation rather than a legitimate buffer.
constexpr size_t kMaxSingleAllocation = 256 * 1024 * 1024;

class JimboAllocator : public MemFunctions
{
public:
    JimboAllocator();
    ~JimboAllocator();

    void* New(size_t size) override;
    void Delete(void* mem) override;
    void* HeapBase() override;
    size_t HeapUsed() override;
    int FreeBlocks() override;
    int MemoryAllocatedBlocks() override;
    bool CheckIntegrity() override;
    bool IsOutOfMemory() override;
    void CleanUp() override;

    // FreeOnDemand integration
    void RegisterFreeOnDemand(IMemoryFreeOnDemand* object);
    size_t FreeOnDemand(size_t size);
    size_t FreeOnDemandAll();

    // Process-wide budget. Both limits default to 0 = unlimited.
    void SetBudgetLimits(size_t softBytes, size_t hardBytes) { _budget.SetLimits(softBytes, hardBytes); }
    bool OverSoftLimit() const { return _budget.OverSoftLimit(); }
    size_t SoftLimit() const { return _budget.SoftLimit(); }
    size_t HardLimit() const { return _budget.HardLimit(); }
    size_t PeakUsed() const { return _budget.Peak(); }
    void ResetPeak() { _budget.ResetPeak(); }

    // Per-subsystem residency snapshot (walks the FreeOnDemand registry).
    int SnapshotDomains(MemoryDomainStat* out, int maxOut) { return _freeOnDemand.Snapshot(out, maxOut); }
    size_t EnforceBudgets() { return _freeOnDemand.EnforceBudgets(); }

    // Once-per-frame memory maintenance, called off the allocation path. Below the
    // soft watermark (and under any hard ceiling) it is a few atomic loads and
    // returns. Over soft it trims each domain to its declared budget; over hard it
    // additionally claws the overflow back with cost-ordered FreeOnDemand eviction.
    // Returns bytes freed this tick. This is where crossing a limit turns into
    // eviction — Reserve() itself never evicts.
    size_t FrameMaintenance();

private:
    mi_heap_t* _heap;
    std::atomic<size_t> _allocCount;
    std::atomic<bool> _outOfMemory;
    ProcessMemoryBudget _budget;
    MemoryFreeOnDemandList _freeOnDemand;
};

// Explicit-locking variant; mimalloc is already thread-safe, this is for API compatibility.
class JimboAllocatorLocked : public JimboAllocator
{
public:
    JimboAllocatorLocked() = default;
    ~JimboAllocatorLocked() = default;

    void* New(size_t size) override;
    void Delete(void* mem) override;
    void CleanUp() override;
};

// Global function to register FreeOnDemand handlers with allocator.
// (Process-budget free functions are declared in MemFreeReq.hpp so callers can
// use them without pulling in mimalloc.)
void RegisterMemoryFreeOnDemand(IMemoryFreeOnDemand* object);

} // namespace Poseidon::Foundation

#endif // POSEIDON_MEMORY_JIMBO_ALLOCATOR_HPP
