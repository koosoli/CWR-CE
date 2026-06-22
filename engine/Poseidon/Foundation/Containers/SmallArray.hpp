#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/ConstructTraitsModern.hpp>

#define _small ((Type*)_smallSpace)


namespace Poseidon::Foundation
{
template <class Type, int size = 64>
class VerySmallArray
{
    typedef Poseidon::Foundation::ModernTraits<Type> CTraits;

    enum
    {
        MaxSmall = (size - 4) / sizeof(Type)
    };
    int _nSmall{0};
    // alignas(Type): _small reinterprets this storage as Type*, so it must carry
    // Type's alignment — without it an 8-byte-aligned Type (e.g. InitPtr<...>)
    // binds references to a 4-byte-aligned address after _nSmall (UBSan).
    alignas(Type) unsigned char _smallSpace[MaxSmall * sizeof(Type)];

  public:
    VerySmallArray();
    ~VerySmallArray() { Clear(); }

    const Type* Data() const { return _small; }
    const Type& Get(int i) const { return _small[i]; }
    Type& Set(int i) { return _small[i]; }

    const Type& operator[](int i) const { return _small[i]; }
    Type& operator[](int i) { return _small[i]; }

    int Add();
    int Add(const Type& object);
    void Delete(int index);
    void Clear();
    int Size() const { return _nSmall; }

};

template <class Type, int size>
VerySmallArray<Type, size>::VerySmallArray() = default;

template <class Type, int size>
int VerySmallArray<Type, size>::Add(const Type& object)
{
    if (_nSmall < MaxSmall)
    {
        CTraits::CopyConstruct(_small[_nSmall], object);
        return _nSmall++;
    }
    return -1;
}

template <class Type, int size>
int VerySmallArray<Type, size>::Add()
{
    if (_nSmall < MaxSmall)
    {
        CTraits::Construct(_small[_nSmall]);
        return _nSmall++;
    }
    return -1;
}

template <class Type, int size>
void VerySmallArray<Type, size>::Delete(int index)
{
    CTraits::Destruct(_small[index]);
    CTraits::DeleteData(_small + index, _nSmall - index, 1);
    _nSmall--;
}

template <class Type, int size>
void VerySmallArray<Type, size>::Clear()
{
    CTraits::DestructArray(_small, _nSmall);
    _nSmall = 0;
}

#undef _small

template <class Type, int size = 64>
class SmallArray
{
    // sizeof( AutoArray ) is 4 DWORDS
    // try to allign to 16 DWORDS (64 B)
    // we have 44 bytes to contain Type elements
    VerySmallArray<Type, size - 16> _small;
    AutoArray<Type> _large;

  public:
    SmallArray();
    ~SmallArray() { Clear(); }

    int Size() const { return _small.Size() + _large.Size(); }
    const Type& Get(int i) const
    {
        if (i < _small.Size())
        {
            return _small.Get(i);
        }
        else
        {
            return _large.Get(i - _small.Size());
        }
    }
    Type& Set(int i)
    {
        if (i < _small.Size())
        {
            return _small.Set(i);
        }
        else
        {
            return _large.Set(i - _small.Size());
        }
    }

    const Type& operator[](int i) const { return Get(i); }
    Type& operator[](int i) { return Set(i); }

    int Add(const Type& object);
    void Delete(int index);
    void Compact();
    void Clear();

};

template <class Type, int size>
SmallArray<Type, size>::SmallArray() = default;

template <class Type, int size>
int SmallArray<Type, size>::Add(const Type& object)
{
    int index = _small.Add(object);
    if (index >= 0)
    {
        return index;
    }
    return _large.Add(object) + _small.Size();
}

template <class Type, int size>
void SmallArray<Type, size>::Delete(int index)
{
    if (index < _small.Size())
    {
        _small.Delete(index);
    }
    else
    {
        _large.Delete(index - _small.Size(), 1);
    }
}

template <class Type, int size>
void SmallArray<Type, size>::Compact()
{
    _large.Compact();
}

template <class Type, int size>
void SmallArray<Type, size>::Clear()
{
    _small.Clear();
    _large.Clear();
}

#undef _small

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::VerySmallArray;
using ::Poseidon::Foundation::SmallArray;
