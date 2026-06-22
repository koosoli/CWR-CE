#include <Poseidon/Foundation/Platform/FPUSetup.hpp>
#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Dev/Debug/DebugTrap.hpp>

using namespace Poseidon::Dev;
#ifndef _WIN32
#include <cfenv>

#endif
extern void SetFlushToZero();

namespace Poseidon::Foundation
{

void InitFPU()
{
#ifdef _WIN32
    // x64: SSE math, no precision control — only rounding + exception masking.
    if (GDebugger.IsDebugger())
    {
        _control87(RC_NEAR | MCW_EM & ~0, MCW_EM | MCW_RC);
    }
    else
    {
        _control87(RC_NEAR | MCW_EM, MCW_EM | MCW_RC);
    }
#ifdef _KNI
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    if (GDebugger.IsDebugger())
    {
        PoseidonAssert(_MM_GET_EXCEPTION_MASK() == _MM_MASK_MASK);
        _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK & ~(
                                                   //_MM_MASK_INVALID|
                                                   //_MM_MASK_DENORM|
                                                   //_MM_MASK_DIV_ZERO|
                                                   //_MM_MASK_OVERFLOW|_MM_MASK_UNDERFLOW|
                                                   //_MM_MASK_INEXACT|
                                                   0));
    }
    else
    {
        // disable all exceptions - should be default
        _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK);
    }
#endif
    if (ENGINE_CONFIG.enablePIII)
    {
        SetFlushToZero();
    }
#else // !_WIN32
    fesetround(FE_TONEAREST);
#endif
}

} // namespace Poseidon::Foundation
