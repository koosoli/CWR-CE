#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>

using Poseidon::Foundation::MemFunctions;

static MemFunctions defaultMemFunctions;
MemFunctions* GMemFunctionsPtr = &defaultMemFunctions;
MemFunctions* GSafeMemFunctionsPtr = &defaultMemFunctions;

// selectany: wins over JimboAllocator's selectany because test objects are linked first
class PoseidonBaseTestAppFrameFunctions : public AppFrameFunctions
{
};

static PoseidonBaseTestAppFrameFunctions testAppFrame;
#ifdef _MSC_VER
__declspec(selectany)
#elif defined(__GNUC__)
__attribute__((weak))
#endif
Poseidon::Foundation::AppFrameFunctions* Poseidon::Foundation::CurrentAppFrameFunctions = &testAppFrame;

// d5q-PoseidonGL33 stubs: test binaries don't link PoseidonGL33.lib;
// provide null stubs for the two functions tests' includes reference.
namespace Poseidon
{
class Engine;
}
extern "C"
{
} // ensure C++ context
namespace Poseidon
{
class Engine* CreateEngineGL33(int, int, bool, int)
{
    return nullptr;
}
} // namespace Poseidon
namespace Poseidon
{
void RegisterGL33GraphicsBackend() {}
} // namespace Poseidon
