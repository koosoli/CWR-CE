#pragma once

// Minimal headless engine init shared by the asset-format fuzz harnesses.
//
// The format parsers (P3D/PAA/ParamFile) build plain structs from a byte buffer
// but route their error/warning reports through the global AppFrame and allocate
// engine strings/containers. Install a silent app frame (so a malformed-input
// error path is a no-op rather than a null-deref) and mark the memory system +
// ParamFile library element ready -- the same minimal setup the PoseidonFormats
// C API DLL does in pf_init(), minus the dummy graphics engine the parsers never
// touch. Call fuzz::InitEngine() once from LLVMFuzzerInitialize.

#include <Poseidon/Foundation/Framework/AppFrame.hpp>
#include <Poseidon/Foundation/Platform/InitBridge.hpp>
#include <Poseidon/IO/ParamFile/InitLibraryElement.hpp>

#include <cstdarg>

namespace fuzz
{
class SilentAppFrame : public Poseidon::Foundation::AppFrameFunctions
{
  public:
    void ErrorMessage(Poseidon::Foundation::ErrorMessageLevel, const char*, va_list) override {}
    void ErrorMessage(const char*, va_list) override {}
    void WarningMessage(const char*, va_list) override {}
    void ShowMessage(int, const char*, va_list) override {}
    DWORD TickCount() override { return 0; }
};

inline void InitEngine()
{
    static SilentAppFrame frame;
    Poseidon::Foundation::CurrentAppFrameFunctions = &frame;
    ::SetMemorySystemReady(true);
    Poseidon::InitLibraryElement();
}
} // namespace fuzz
