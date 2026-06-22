#pragma once

#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/platform.hpp>
namespace Poseidon::Foundation
{
// 2D view over a 1D AutoArray, row-major (index = y * xRange + x).
template <class Type, class Allocator = Poseidon::Foundation::MemAllocD, class Array1D = AutoArray<Type, Allocator>>
class Array2D
{
    Array1D _data;
    int _xRng;
    int _yRng;

  public:
    Array2D();
    ~Array2D();

    void Dim(int x, int y);
    int GetXRange() const { return _xRng; }
    int GetYRange() const { return _yRng; }

    void Clear()
    {
        _xRng = _yRng = 0;
        _data.Clear();
    }
    __forceinline Type& Set(int x, int y) { return _data[y * _xRng + x]; }
    __forceinline const Type& Get(int x, int y) const { return _data[y * _xRng + x]; }
    __forceinline Type& operator()(int x, int y) { return Set(x, y); }
    __forceinline const Type& operator()(int x, int y) const { return Get(x, y); }
    const void* RawData() const { return _data.Data(); }
    void* RawData() { return _data.Data(); }
    int RawSize() const { return _data.Size() * sizeof(Type); }
};

template <class Type, class Allocator, class Array1D>
void Array2D<Type, Allocator, Array1D>::Dim(int x, int y)
{
    _xRng = x;
    _yRng = y;
    _data.Realloc(x * y);
    _data.Resize(x * y);
}

template <class Type, class Allocator, class Array1D>
Array2D<Type, Allocator, Array1D>::Array2D()
{
    _xRng = _yRng = 0;
}

template <class Type, class Allocator, class Array1D>
Array2D<Type, Allocator, Array1D>::~Array2D()
{
    Clear();
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::Array2D;
