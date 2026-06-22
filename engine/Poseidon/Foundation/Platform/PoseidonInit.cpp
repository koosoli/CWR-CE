#include <Poseidon/Foundation/Platform/PoseidonInit.hpp>

// Forward declarations of each layer's explicit Init function.  Kept
// here rather than in scattered headers so this file is the single
// grep point for "what gets registered at startup".

extern void InitScriptingDefaults();      // ExpressExt.cpp
extern void InitParamFileExtDefaults();   // paramFileExt.cpp
extern void InitProgressSystemDefaults(); // ProgressSystem.cpp
extern void InitClipboardFunctions();     // qstreamExt.cpp

namespace Poseidon
{
void InitDefaults()
{
    // Order is intentional, but only insofar as each Init*() is
    // self-contained — no Init below this point depends on any Init
    // above, and re-running any of them is a no-op (each just calls
    // a Set*Functions setter with a static-storage instance).
    InitScriptingDefaults();
    InitParamFileExtDefaults();
    InitProgressSystemDefaults();
    InitClipboardFunctions();
}
} // namespace Poseidon
