#include <Poseidon/Core/Application.hpp>

#include <Poseidon/Core/Config/EngineConfig.hpp>
#include <Poseidon/Foundation/Math/V3Quads.hpp>
#include <Poseidon/Graphics/Rendering/Shape/Shape.hpp>
#include <stdint.h>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>

// SoAoS: Structure of arrays of structures

namespace Poseidon::Foundation
{
V3Array::V3Array()
{
    _size = 0;
}

V3Array::~V3Array() = default;

const V3QElement V3QZero(0, 0, 0);
const V3QElement V3QAside(1, 0, 0);
const V3QElement V3QUp(0, 1, 0);
const V3QElement V3QForward(0, 0, 1);

void V3Array::SetStorage(MemAllocS* storage)
{
    void* data = storage->GetMemory();
    if (((intptr_t)data & 15) != 0)
    {
        LOG_DEBUG(Core, "Static storage not aligned - {:x}", (uintptr_t)data);
        Fail("  misaligned data");
    }
    _data.SetStorage(storage);
}

inline bool Aligned(const void* x, int a)
{
    return (intptr_t(x) & (a - 1)) == 0;
}

#define AssertAligned(x, a)                                            \
    if (!Aligned(x, a))                                                \
    {                                                                  \
        LOG_DEBUG(Core, "Alignment error: {} {:x}", #x, (uintptr_t)x); \
        Fail("Misaligned data");                                       \
    }

} // namespace Poseidon::Foundation

#if USE_QUADS
void ::Poseidon::VertexTable::ConvertToQArray()
{
    if (!ENGINE_CONFIG.enablePIII)
    {
        return;
    }
    int n = _pos.Size();
    _posQ.Realloc(n);
    _normQ.Realloc(n);
    _posQ.Resize(n);
    _normQ.Resize(n);

    AssertAligned(_posQ.QuadData(), 16);
    AssertAligned(_normQ.QuadData(), 16);

    for (int i = 0; i < n; i++)
    {
        _posQ.Set(i) = _pos[i];
        _normQ.Set(i) = _norm[i];
    }
}

void ::Poseidon::VertexTable::RemoveNormalArrays()
{
    _pos.Clear();
    _norm.Clear();
    _orig.Clear();
    _origNorm.Clear();
}

#endif
