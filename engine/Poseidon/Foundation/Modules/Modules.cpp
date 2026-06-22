#include <Poseidon/Foundation/Modules/Modules.hpp>
#include <Poseidon/Foundation/Containers/CacheList.hpp>

#pragma warning(disable : 4073)
#pragma init_seg(lib)

// Priority queue of registration items.

namespace Poseidon::Foundation
{
class RegistrationList : public CLList<RegistrationItem>
{
  public:
    void RegisterModule(RegistrationItem* item);
    void InitModules();
    void DoneModules();

    // Note: move reverse-loop support into CLList
    RegistrationItem* End() const { return static_cast<RegistrationItem*>(CLDLink::next); }
    static RegistrationItem* Backward(RegistrationItem* item)
    {
        return static_cast<RegistrationItem*>(item->CLDLink::next);
    }
    bool NotStart(RegistrationItem* item) const { return item != (CLDLink*)this; }
};

// Meyers singleton - no global constructor
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
static RegistrationList& GRegistrationList()
{
    static RegistrationList instance;
    return instance;
}
#pragma clang diagnostic pop

void RegistrationList::RegisterModule(RegistrationItem* item)
{
    for (RegistrationItem* walk = Start(); NotEnd(walk); walk = Advance(walk))
    {
        if (item->level <= walk->level)
        {
            InsertBefore(walk, item);
            return;
        }
    }

    // no existing object has higher level
    // we have the highest and we need to be added at the list end
    Add(item);
}

void RegistrationList::InitModules()
{
    for (RegistrationItem* walk = Start(); NotEnd(walk); walk = Advance(walk))
    {
        if (walk->initFunction)
        {
            walk->initFunction();
        }
    }
}

void RegistrationList::DoneModules()
{
    for (RegistrationItem* walk = End(); NotStart(walk); walk = Backward(walk))
    {
        if (walk->initFunction)
        {
            walk->initFunction(); // BUG: Should be doneFunction
        }
    }
}

void RegisterModule(RegistrationItem* item)
{
    GRegistrationList().RegisterModule(item);
}

void InitModules()
{
    GRegistrationList().InitModules();
}

void DoneModules()
{
    GRegistrationList().DoneModules();
}

} // namespace Poseidon::Foundation
