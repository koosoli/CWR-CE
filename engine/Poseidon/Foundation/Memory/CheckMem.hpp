#pragma once

// MemFunctions: the abstract allocator interface. A concrete implementation
// (JimboAllocator) is bound to the global pointers below at static-init time.

#ifndef CCALL
#define CCALL
#include <cstddef>
#include <cstdlib>
#endif

#include <cstdint>

namespace Poseidon::Foundation
{
class MemFunctions
{
  public:
    virtual void* New(size_t size) { return malloc(size); }
    virtual void Delete(void* mem) { free(mem); }

    // base of the allocation space; null when unknown/undefined
    virtual void* HeapBase() { return nullptr; }

    // approximate committed bytes
    virtual size_t HeapUsed() { return 0; }

    // approximate free-block count (fragmentation signal)
    virtual int FreeBlocks() { return 1; }

    // approximate allocated-block count (fragmentation signal)
    virtual int MemoryAllocatedBlocks() { return 1; }

    virtual bool CheckIntegrity() { return true; }

    // set once memory is exhausted; callers should back off allocating
    virtual bool IsOutOfMemory() { return false; }

    // release any cached memory
    virtual void CleanUp() {}
};

// Late binding: Poseidon defines these with #pragma init_seg(compiler).
extern MemFunctions* GMemFunctionsPtr;
extern MemFunctions* GSafeMemFunctionsPtr;

inline MemFunctions* GMemFunctions()
{
    return GMemFunctionsPtr;
}

inline MemFunctions* GSafeMemFunctions()
{
    return GSafeMemFunctionsPtr;
}

} // namespace Poseidon::Foundation

// Placement new (address cast) — must be at global scope
inline void* CCALL operator new(size_t size, uintptr_t addr, char const*, int)
{
    (void)size;
    return (void*)addr;
}

// Global operator new/delete replacements — must be at global scope
void* CCALL operator new(size_t size);
void* CCALL operator new[](size_t size);
void CCALL operator delete(void* ptr) noexcept;
void CCALL operator delete[](void* ptr) noexcept;

namespace Poseidon::Foundation
{
// required interface for MT safe heap
inline void* safeNew(size_t size)
{
    return GSafeMemFunctions()->New(size);
}
inline void safeDelete(void* mem)
{
    GSafeMemFunctions()->Delete(mem);
}

inline bool IsOutOfMemory()
{
    return GMemFunctions()->IsOutOfMemory();
}
inline size_t MemoryUsed()
{
    return GMemFunctions()->HeapUsed();
}

inline void SafeMemoryCleanUp()
{
    GSafeMemFunctions()->CleanUp();
}
inline int MemoryFreeBlocks()
{
    return GMemFunctions()->FreeBlocks();
}
} // namespace Poseidon::Foundation

