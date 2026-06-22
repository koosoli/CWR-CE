#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>
#include <limits.h>
#include <stdint.h>
#include <typeinfo>
#include <Poseidon/Foundation/Framework/Log.hpp>

namespace Poseidon::Foundation
{
size_t MemoryFreeOnDemandHelper::Free(size_t ammount)
{
    size_t freedTotal = 0;
    for (;;)
    {
        size_t freed = FreeOneItem();
        if (freed == 0)
        {
            break;
        }
        freedTotal += freed;
        if (ammount <= freedTotal)
        {
            break;
        }
    }
    return freedTotal;
}

size_t MemoryFreeOnDemandHelper::FreeAll()
{
    return Free(UINT_MAX);
}

void MemoryFreeOnDemandList::Register(IMemoryFreeOnDemand* object)
{
    // the list is kept sorted by priority, lowest first
    for (IMemoryFreeOnDemand* walk = Start(); NotEnd(walk); walk = Advance(walk))
    {
        if (object->Priority() < walk->Priority())
        {
            InsertBefore(walk, object);
            return;
        }
    }

    // nothing has higher priority — this object goes at the end
    Add(object);
}

size_t MemoryFreeOnDemandList::Free(size_t ammount)
{
    size_t freedTotal = 0;
    for (IMemoryFreeOnDemand* walk = Start(); NotEnd(walk); walk = Advance(walk))
    {
        size_t freed = walk->Free(ammount);
        freedTotal += freed;
        if (freed > 0)
        {
            LOG_DEBUG(Core, "{:x}:{}: Flush of {} B", (uintptr_t)walk, typeid(*walk).name(), freed);
        }
        if (ammount <= freedTotal)
        {
            break;
        }
    }

    return freedTotal;
}

size_t MemoryFreeOnDemandList::FreeAll()
{
    int freedTotal = 0;
    for (IMemoryFreeOnDemand* walk = Start(); NotEnd(walk); walk = Advance(walk))
    {
        int freed = walk->FreeAll();
        freedTotal += freed;
    }

    return freedTotal;
}

int MemoryFreeOnDemandList::Snapshot(MemoryDomainStat* out, int maxOut)
{
    int n = 0;
    for (IMemoryFreeOnDemand* walk = Start(); NotEnd(walk) && n < maxOut; walk = Advance(walk))
    {
        out[n].name = walk->DomainName();
        out[n].heldBytes = walk->HeldBytes();
        out[n].budgetBytes = walk->BudgetBytes();
        out[n].heldItems = walk->HeldItems();
        out[n].priority = walk->Priority();
        n++;
    }
    return n;
}

size_t MemoryFreeOnDemandList::EnforceBudgets()
{
    size_t freedTotal = 0;
    for (IMemoryFreeOnDemand* walk = Start(); NotEnd(walk); walk = Advance(walk))
    {
        const size_t budget = walk->BudgetBytes();
        if (budget == 0)
        {
            continue;
        }
        const size_t held = walk->HeldBytes();
        if (held > budget)
        {
            freedTotal += walk->Free(held - budget);
        }
    }
    return freedTotal;
}

} // namespace Poseidon::Foundation
