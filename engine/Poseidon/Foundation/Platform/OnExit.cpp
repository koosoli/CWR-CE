#include <Poseidon/Foundation/platform.hpp>

#pragma warning(disable : 4074)
#pragma init_seg(compiler)

namespace Poseidon::Foundation
{
extern void MemoryInit();
extern void MemoryDone();

class OnExit
{
  public:
    OnExit() = default; // MemoryInit is called explicitly from WinMain, not here
    ~OnExit() { MemoryDone(); }
};

static OnExit Exit INIT_PRIORITY_URGENT;

} // namespace Poseidon::Foundation
