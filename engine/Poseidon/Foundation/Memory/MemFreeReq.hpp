#ifdef _MSC_VER
#pragma once
#endif

#ifndef _MEM_FREE_REQ_HPP
#define _MEM_FREE_REQ_HPP

#include <Poseidon/Foundation/Containers/CacheList.hpp>
#include <functional>
#include <utility>

// Interface for an on-demand deallocator. When an allocation fails, the heap walks
// its registered deallocators (caches, banks, ...) to reclaim memory and retry.


namespace Poseidon::Foundation
{
class IMemoryFreeOnDemand : public CLDLink
{
  public:
    // Free at least the requested amount; returns the bytes actually freed.
    virtual size_t Free(size_t ammount) = 0;
    // Free as much as possible; returns the bytes actually freed.
    virtual size_t FreeAll() = 0;
    // Reclaim cost: estimated CPU cycles to recreate one byte of the controlled
    // objects. Lowest-priority registrants are freed first. Read at registration.
    virtual float Priority() = 0;

    // short human-readable label for the memory-budget panel (e.g. "FileCache")
    virtual const char* DomainName() const { return "?"; }

    // bytes this subsystem currently holds, if cheaply known (0 = unknown)
    virtual size_t HeldBytes() const { return 0; }

    // soft residency budget in bytes (0 = none declared). When HeldBytes()
    // exceeds this, MemoryFreeOnDemandList::EnforceBudgets() trims the overflow.
    virtual size_t BudgetBytes() const { return 0; }

    // item count this subsystem holds, for caches with no cheap byte size
    // (e.g. the shape/model bank). 0 = not reported. Pure observability.
    virtual size_t HeldItems() const { return 0; }

    virtual ~IMemoryFreeOnDemand() = default;
};

// One subsystem's residency, for the memory-budget snapshot / dev panel.
struct MemoryDomainStat
{
    const char* name = "?";
    size_t heldBytes = 0;
    size_t budgetBytes = 0;
    size_t heldItems = 0;
    float priority = 0.f;
};

extern void RegisterMemoryFreeOnDemand(IMemoryFreeOnDemand* object);

// Adapter that lets a class participate in the FreeOnDemand registry WITHOUT
// inheriting IMemoryFreeOnDemand — for banks that already have their own base
// (TextBankGL33, ShapeBank) or want to stay decoupled (PcmCache). Hold one as a
// member and call Register() once; it auto-unlinks when destroyed (CLTLink dtor).
class MemoryDomainProbe : public IMemoryFreeOnDemand
{
  public:
    using BytesFn = std::function<size_t()>;
    using FreeFn  = std::function<size_t(size_t)>; // empty = non-evicting (observability only)

    MemoryDomainProbe() = default;

    // `name` must outlive the probe (pass a string literal). `freeFn` may be
    // empty for caches that can't be safely evicted by the global pressure path.
    void Register(const char* name, float priority, BytesFn heldBytes, BytesFn budgetBytes = {},
                  BytesFn heldItems = {}, FreeFn freeFn = {})
    {
        _name   = name;
        _prio   = priority;
        _held   = std::move(heldBytes);
        _budget = std::move(budgetBytes);
        _items  = std::move(heldItems);
        _free   = std::move(freeFn);
        RegisterMemoryFreeOnDemand(this);
    }

    size_t Free(size_t amount) override { return _free ? _free(amount) : 0; }
    size_t FreeAll() override { return _free ? _free(~size_t(0)) : 0; }
    float Priority() override { return _prio; }
    const char* DomainName() const override { return _name; }
    size_t HeldBytes() const override { return _held ? _held() : 0; }
    size_t BudgetBytes() const override { return _budget ? _budget() : 0; }
    size_t HeldItems() const override { return _items ? _items() : 0; }

  private:
    const char* _name = "?";
    float       _prio = 1.0f;
    BytesFn     _held, _budget, _items;
    FreeFn      _free;
};

// List of registered on-demand deallocators, kept sorted by priority (lowest first).
class MemoryFreeOnDemandList : public CLList<IMemoryFreeOnDemand>
{
  public:
    void Register(IMemoryFreeOnDemand* object);
    size_t Free(size_t ammount);
    size_t FreeAll();

    // Fill `out` with up to `maxOut` per-subsystem residency entries (in
    // reclaim-priority order). Returns the number written.
    int Snapshot(MemoryDomainStat* out, int maxOut);

    // Trim every registrant that declared a BudgetBytes() back down to it.
    // Returns total bytes freed. Registrants with no declared budget are skipped.
    size_t EnforceBudgets();
};

class MemoryFreeOnDemandHelper : public IMemoryFreeOnDemand
{
  public:
    // free one item, preferably the cheapest one
    virtual size_t FreeOneItem() = 0;

    // Free/FreeAll are built on top of FreeOneItem
    size_t Free(size_t ammount) override;
    size_t FreeAll() override;
};

extern void RegisterMemoryFreeOnDemand(IMemoryFreeOnDemand* object);

// --- Process-wide memory budget (defined in JimboAllocator.cpp) ---------------
// Declared here (not in JimboAllocator.hpp) so callers can use the budget API
// without pulling mimalloc into their TU.

// Coarse process-wide limits (bytes). 0 = unlimited. A safety net + soft-pressure
// signal, not the primary per-system policy (that lives in this registry).
void SetProcessMemoryLimits(size_t softBytes, size_t hardBytes);

struct ProcessMemoryStats
{
    size_t used = 0;
    size_t peak = 0;
    size_t softLimit = 0;
    size_t hardLimit = 0;
    int registrants = 0;
};
ProcessMemoryStats MemoryProcessStats();

// Fill `out` with up to `maxOut` per-subsystem residency entries. Returns count.
int MemorySnapshotDomains(MemoryDomainStat* out, int maxOut);

// Trim every registered subsystem back to its declared budget. Returns bytes freed.
size_t MemoryEnforceBudgets();

// Once-per-frame memory maintenance, called from the frame loop off the allocation
// path. Below the soft watermark it is a few atomic loads and returns 0. Over soft
// it trims each domain to its declared budget; over the hard ceiling it additionally
// claws the overflow back with cost-ordered eviction. Returns bytes freed this tick.
size_t MemoryFrameMaintenance();

// Total physical RAM in bytes (0 if it can't be determined). Used to derive the
// default process limits (DeriveDefaultMemoryLimits) when config leaves them auto.
size_t QueryPhysicalMemoryBytes();

} // namespace Poseidon::Foundation

#endif
