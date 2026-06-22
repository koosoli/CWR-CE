// Global operator new/delete redirected to the custom allocator (BMemory).
//
// IMPORTANT: this file must NOT use #pragma init_seg(compiler) — it would
// interfere with BMemory static initialization.

#include <cstdlib>
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#include <Poseidon/Foundation/platform.hpp>

// GMemFunctions() and operator declarations are in Poseidon/Foundation/Memory/CheckMem.hpp (via PoseidonPCH.hpp)

// Undefine debug new macro so we can define the actual operators
#undef new

// Under AddressSanitizer the custom heap (BMemory) hides every allocation
// behind one big malloc, so ASan cannot place redzones around individual
// objects or track per-object alloc/free provenance. Route every global
// new/delete straight to malloc/free so ASan instruments each object.
// This is sanitizer-build-only; production keeps the custom heap.
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define POSEIDON_ASAN_GLOBAL_NEW 1
#endif
#endif

#ifndef POSEIDON_ASAN_GLOBAL_NEW

// Guard: during early dynamic library init (e.g. OpenAL), GMemFunctionsPtr may
// exist but not be fully constructed (vtable still null). Fall back to malloc/free.

namespace Poseidon
{
static inline bool GMemReady()
{
    if (!Foundation::GMemFunctionsPtr)
        return false;
    // Check vtable pointer (first member of the object)
    const void* vtable = *reinterpret_cast<void* const*>(Foundation::GMemFunctionsPtr);
    return vtable != nullptr;
}
} // namespace Poseidon

// Global operator new/delete implementations (must be at global scope)
void* CCALL operator new(size_t size)
{
    if (!Poseidon::GMemReady())
        return malloc(size);
    return Poseidon::Foundation::GMemFunctions()->New(size);
}

void* CCALL operator new[](size_t size)
{
    if (!Poseidon::GMemReady())
        return malloc(size);
    return Poseidon::Foundation::GMemFunctions()->New(size);
}

void CCALL operator delete(void* ptr) noexcept
{
    if (!Poseidon::GMemReady())
    {
        free(ptr);
        return;
    }
    Poseidon::Foundation::GMemFunctions()->Delete(ptr);
}

void CCALL operator delete[](void* ptr) noexcept
{
    if (!Poseidon::GMemReady())
    {
        free(ptr);
        return;
    }
    Poseidon::Foundation::GMemFunctions()->Delete(ptr);
}

// Sized delete operators (C++14)
void CCALL operator delete(void* ptr, size_t size) noexcept
{
    (void)size;
    if (!Poseidon::GMemReady())
    {
        free(ptr);
        return;
    }
    Poseidon::Foundation::GMemFunctions()->Delete(ptr);
}

void CCALL operator delete[](void* ptr, size_t size) noexcept
{
    (void)size;
    if (!Poseidon::GMemReady())
    {
        free(ptr);
        return;
    }
    Poseidon::Foundation::GMemFunctions()->Delete(ptr);
}

#else // POSEIDON_ASAN_GLOBAL_NEW

// Define nothing: let AddressSanitizer's own operator new/delete own every
// global allocation. They route straight through ASan's instrumented allocator
// (native redzones, per-object alloc/free provenance) — strictly better than a
// malloc passthrough — and the ASan static runtime thunk expects to intercept
// them, so a competing definition here trips lld-link ("operator new was
// replaced"). This is sanitizer-build-only; production keeps the custom heap.

#endif // POSEIDON_ASAN_GLOBAL_NEW
