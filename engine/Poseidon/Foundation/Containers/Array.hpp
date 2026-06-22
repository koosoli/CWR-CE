#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/TypeOpts.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

#ifdef _CPPRTTI
#include <typeinfo>
#endif

#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Containers/ConstructTraitsModern.hpp>

namespace Poseidon::Foundation
{
// Growable array with explicit construct/destruct control (via ModernTraits).
template <class Type, class Allocator = Poseidon::Foundation::MemAllocD>
class AutoArray
{
    typedef Poseidon::Foundation::ModernTraits<Type> CTraits;

  protected:
    Allocator _alloc; // often zero-sized
    Type* _data;
    int _maxN; // allocated capacity; always >= _n
    int _n;    // number of live items; always <= _maxN
    void DoConstruct();
    void DoDestruct();

  public:
    // Smallest capacity bump applied when the array grows.
    enum
    {
        MinGrow = 32
    };
    AutoArray();
    explicit AutoArray(int n);
    explicit AutoArray(const Allocator& alloc)
    {
        DoConstruct();
        SetStorage(alloc);
    }
    AutoArray(const Type* src, int n);
    AutoArray(const AutoArray& src);
    ~AutoArray();
    void operator=(const AutoArray& src);
    // Fast move: transfer src's buffer into this; src is left empty.
    void Move(AutoArray& src);
    void Clear() { DoDestruct(); }
    void Copy(const Type* src, int n);
    // Allocate room for n items via the array's allocator. n is in/out: it is updated if the
    // allocator rounded the request up to a larger block.
    Type* MemAlloc(int& n)
    {
        int size = n * sizeof(Type), size0 = size;
#if ALLOC_DEBUGGER && defined _CPPRTTI
        Type* ret = (Type*)_alloc.Alloc(size, __FILE__, __LINE__, typeid(Type).name());
#else
        Type* ret = (Type*)_alloc.Alloc(size);
#endif
        if (size != size0)
        {
            n = size / sizeof(Type);
        }
        return ret;
    }
    void SetStorage(const Allocator& alloc) { _alloc = alloc; }
    // Detach the current allocator; the data stays valid under default management.
    void UnlinkStorage() { _alloc.Unlink(_data); }
    void MemFree(Type* mem, int n) { _alloc.Free(mem, n * sizeof(Type)); }
    void Init(int n);
    // Grow so index n is valid: Access(i) == Resize(i + 1).
    void Access(int n);
    // Grow capacity to sizeNeed when smaller; never shrinks (faster than Realloc).
    void Reserve(int sizeNeed, int sizeWant);
    // Reallocate only when capacity is outside [sizeNeed, sizeWant]; new capacity becomes sizeWant.
    void Realloc(int sizeNeed, int sizeWant);
    void Realloc(int size);
    // Set the live count to n, constructing/destructing items and growing capacity as needed.
    void Resize(int n);
    // Set the live count to n without construct/destruct control. Dangerous; caller owns lifetimes.
    void UnsafeResize(int n);

    // Shrink capacity to the current item count.
    void Compact() { Realloc(_n); }
    void InitEmpty()
    {
        DestructArray(_data, _n);
        ConstructArray(_data, _n);
    }
    // Append without growing; caller guarantees spare capacity (_n < _maxN). Returns the index.
    int AddFast(const Type& src)
    {
        PoseidonAssert(_n < _maxN);
        CTraits::CopyConstruct(_data[_n], src);
        return _n++;
    }
    int AddFast()
    {
        PoseidonAssert(_n < _maxN);
        CTraits::Construct(_data[_n]);
        return _n++;
    }
    // Append, growing capacity if needed. Returns the new item's index.
    int Add(const Type& src);
    int Add();
    // Append a default item and return a reference to it.
    Type& Append() { return Set(Add()); }
#ifdef _WIN32
    void Delete(int index, int count = 1);
#else
    void Delete(int index, int count);
    inline void Delete(int index) { Delete(index, 1); }
#endif
    void Insert(int index, const Type& src);
    void Insert(int index);
    Type& Set(int i)
    {
        AssertDebug(i >= 0 && i < _n);
        return _data[i];
    }
    const Type& Get(int i) const
    {
        AssertDebug(i >= 0 && i < _n);
        return _data[i];
    }
    Type& operator[](int i) { return Set(i); }
    const Type& operator[](int i) const { return Get(i); }
    // Release-checked element access for indices that originate off the wire. Unlike
    // operator[]/Set/Get (whose AssertDebug compiles out under NDEBUG), this bounds-check
    // survives in shipping builds: it returns nullptr instead of indexing out of range, so
    // callers handling attacker-controlled indices can reject rather than read/write past it.
    Type* AtOrNull(int i) { return (i >= 0 && i < _n) ? &_data[i] : nullptr; }
    const Type* AtOrNull(int i) const { return (i >= 0 && i < _n) ? &_data[i] : nullptr; }
    const Type* Data() const { return _data; }
    Type* Data() { return _data; }
    typedef Type DataType;
    int Size() const { return _n; }
    int MaxSize() const { return _maxN; }
    // Dangerous: prefer QSort from Algorithms/Qsort.hpp. count < 0 sorts the whole array.
    void QSortBin(int (*compare)(const void* v0, const void* v1), int count = -1);
};

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::DoConstruct()
{
    _data = nullptr, _n = _maxN = 0;
}

template <class Type, class Allocator>
AutoArray<Type, Allocator>::~AutoArray()
{
    DoDestruct();
}

template <class Type, class Allocator>
AutoArray<Type, Allocator>::AutoArray()
{
    DoConstruct();
}

template <class Type, class Allocator>
AutoArray<Type, Allocator>::AutoArray(int n)
{
    DoConstruct();
    Realloc(n);
}

template <class Type, class Allocator>
AutoArray<Type, Allocator>::AutoArray(const Type* src, int n)
{
    DoConstruct();
    Copy(src, n);
}

// The copy keeps this array's own allocator rather than the source's.
template <class Type, class Allocator>
AutoArray<Type, Allocator>::AutoArray(const AutoArray& src)
{
    DoConstruct();
    Copy(src.Data(), src.Size());
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::operator=(const AutoArray& src)
{
    Copy(src.Data(), src.Size());
    // the source allocator is intentionally not copied
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Move(AutoArray& src)
{
    Clear();
    _data = src._data;
    _n = src._n;
    _maxN = src._maxN;
    _alloc = src._alloc;
    src._data = nullptr;
    src._n = 0;
    src._maxN = 0;
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Copy(const Type* src, int n)
{
    Reserve(n, n);
    Resize(n);
    CTraits::CopyData(_data, src, _n);
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Realloc(int size)
{
    if (_maxN != size)
    {
        Type* newData = MemAlloc(size);
        if (_data)
        {
            if (_n > size)
            {
                Resize(size);
            }
            CTraits::MoveData(newData, _data, _n);
            MemFree(_data, _maxN);
        }
        _data = newData;
        _maxN = size;
    }
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Realloc(int sizeNeed, int sizeWant)
{
    if (_maxN < sizeNeed || _maxN > sizeWant)
    {
        Realloc(sizeWant);
    }
}

template <class Type, class Allocator>
inline void AutoArray<Type, Allocator>::Reserve(int sizeNeed, int sizeWant)
{
    if (_maxN < sizeNeed)
    {
        Realloc(sizeNeed, sizeWant);
    }
}

template <class Type, class Allocator>
inline void AutoArray<Type, Allocator>::Access(int i)
{
    if (_n < i + 1)
    {
        Resize(i + 1);
    }
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Init(int size)
{
    if (_maxN != size)
    {
        Clear();
        Realloc(size);
    }
    else
    {
        Resize(0);
    }
}

template <class Type, class Allocator>
inline void AutoArray<Type, Allocator>::Resize(int n)
{
    // A negative length is never valid. Without this clamp the shrink path below runs
    // DestructArray(_data + n, _n - n) with the pointer before the buffer and an oversized
    // count, walking off the buffer and destructing garbage — heap corruption reachable from
    // untrusted SQF (`x resize -1`).
    if (n < 0)
    {
        n = 0;
    }
    if (n > _maxN)
    {
        int grow = _maxN;
        if (grow < MinGrow)
        {
            grow = MinGrow;
        }
        Reserve(n, n - 1 + grow);
    }
    if (n > _n)
    {
        CTraits::ConstructArray(_data + _n, n - _n);
    }
    else if (_n > n)
    {
        CTraits::DestructArray(_data + n, _n - n);
    }
    _n = n;
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::UnsafeResize(int n)
{
    if (n > _maxN)
    {
        int grow = _maxN;
        if (grow < MinGrow)
        {
            grow = MinGrow;
        }
        Reserve(n, n - 1 + grow);
    }
    if ((_n = n) < 0)
    {
        _n = 0;
    }
}

template <class Type, class Allocator>
int AutoArray<Type, Allocator>::Add(const Type& src)
{
    if (_n < _maxN)
    {
        return AddFast(src);
    }
    else
    {
        int s = _n;
        Resize(s + 1);
        _data[s] = src;
        return s;
    }
}

template <class Type, class Allocator>
int AutoArray<Type, Allocator>::Add()
{
    if (_n < _maxN)
    {
        return AddFast();
    }
    else
    {
        int s = _n;
        Resize(s + 1);
        return s;
    }
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Delete(int index, int count)
{
    if (index < 0 || index + count > _n)
    {
        Fail("Array index out of range");
        return;
    }
    CTraits::DestructArray(_data + index, count);
    CTraits::DeleteData(_data + index, _n - index, count);
    _n -= count;
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Insert(int index, const Type& src)
{
    Resize(_n + 1); // note: _n is changed by Resize
    CTraits::InsertData(_data + index, _n - index);
    CTraits::CopyConstruct(_data[index], src);
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::Insert(int index)
{
    Resize(_n + 1); // note: _n is changed by Resize
    CTraits::InsertData(_data + index, _n - index);
    CTraits::Construct(_data[index]);
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::QSortBin(int (*compare)(const void* v0, const void* v1), int count)
{
    int size = Size();
    if (count < 0)
        count = size;
    else if (count > size)
        count = size;
    qsort(_data, count, sizeof(_data[0]), compare);
}

template <class Type, class Allocator>
void AutoArray<Type, Allocator>::DoDestruct()
{
    Resize(0);
    if (_data)
    {
        MemFree(_data, _maxN);
        _data = nullptr, _maxN = 0;
    }
}

template <class Type, class Allocator = Poseidon::Foundation::MemAllocD>
class FindArray : public AutoArray<Type, Allocator>
{
  public:
    int AddUnique(const Type& src);
    int Find(const Type& src) const;
    bool Delete(const Type& src);
    void Delete(int index) { AutoArray<Type, Allocator>::Delete(index, 1); }
    void DeleteAt(int index) { AutoArray<Type, Allocator>::Delete(index, 1); }
};

template <class Type, class Allocator>
int FindArray<Type, Allocator>::Find(const Type& src) const
{
    for (int i = 0; i < this->Size(); i++)
    {
        if ((*this)[i] == src)
        {
            return i;
        }
    }
    return -1;
}

template <class Type, class Allocator>
int FindArray<Type, Allocator>::AddUnique(const Type& src)
{
    if (this->Find(src) >= 0)
    {
        return -1;
    }
    int s = this->Size();
    this->Resize(s + 1);
    (*this)[s] = src;
    return s;
}

template <class Type, class Allocator>
bool FindArray<Type, Allocator>::Delete(const Type& src)
{
    int index = Find(src);
    if (index < 0)
    {
        return false;
    }
    Delete(index);
    return true;
}

template <class Type>
struct FindArrayKeyTraits
{
    typedef const Type& KeyType;
    static bool IsEqual(KeyType a, KeyType b) { return a == b; }
    static KeyType GetKey(const Type& a) { return a; }
};

template <class Type, class Traits = FindArrayKeyTraits<Type>, class Allocator = Poseidon::Foundation::MemAllocD>
class FindArrayKey : public AutoArray<Type, Allocator>
{
    typedef typename Traits::KeyType KeyType;

  public:
    int AddUnique(const Type& src);
    int Find(const Type& src) const { return FindKey(Traits::GetKey(src)); }
    int FindKey(KeyType key) const;
    bool Delete(const Type& src) { return DeleteKey(Traits::GetKey(src)); }
    bool DeleteKey(KeyType src);
    void DeleteAt(int index) { AutoArray<Type, Allocator>::Delete(index, 1); }
};

template <class Type, class Traits, class Allocator>
int FindArrayKey<Type, Traits, Allocator>::FindKey(KeyType key) const
{
    for (int i = 0; i < this->Size(); i++)
    {
        if (Traits::IsEqual(Traits::GetKey(this->Get(i)), key))
        {
            return i;
        }
    }
    return -1;
}

template <class Type, class Traits, class Allocator>
int FindArrayKey<Type, Traits, Allocator>::AddUnique(const Type& src)
{
    if (this->Find(src) >= 0)
    {
        return -1;
    }
    int s = this->Size();
    this->Resize(s + 1);
    (*this)[s] = src;
    return s;
}

template <class Type, class Traits, class Allocator>
bool FindArrayKey<Type, Traits, Allocator>::DeleteKey(KeyType key)
{
    int index = FindKey(key);
    if (index < 0)
    {
        return false;
    }
    DeleteAt(index);
    return true;
}

template <class Type, class Allocator = Poseidon::Foundation::MemAllocD>
class RefArray : public FindArrayKey<Ref<Type>, FindArrayKeyTraits<Ref<Type>>, Allocator>
{
    typedef Ref<Type> RefType;
    typedef FindArrayKey<RefType, FindArrayKeyTraits<RefType>, Allocator> base;

  public:
    bool Delete(const RefType& src) { return base::Delete(src); }
    void Delete(int index) { base::DeleteAt(index); }
};

// HeapArray's Type is a pointer (*, Ptr, Ref etc.); operators <, <= must work on *Type.
template <class Type>
struct HeapTraits
{
    // default traits: Type is pointer to actual value
    static bool IsLess(const Type& a, const Type& b) { return *a < *b; }
    static bool IsLessOrEqual(const Type& a, const Type& b) { return *a <= *b; }
};

template <>
struct HeapTraits<int>
{
    // integer heap traits: Type is actual value
    static bool IsLess(int a, int b) { return a < b; }
    static bool IsLessOrEqual(int a, int b) { return a <= b; }
};

template <class Type, class Allocator = Poseidon::Foundation::MemAllocD, class Traits = HeapTraits<Type>>
class HeapArray : public FindArray<Type, Allocator>
{
  public:
    bool HeapRemoveFirst(Type& item);
    void HeapInsert(const Type& item);
    void HeapUpdateUp(const Type& item);
    void HeapDelete(const Type& item);
    void HeapDelete(int index);

  protected:
    void Swap(int i, int j);
    int GetParent(int i);
    int GetFirstSon(int i);
    int GetBrother(int i);
    void HeapDown(int i);
    void HeapUp(int i);
};

template <class Type, class Allocator, class Traits>
inline void HeapArray<Type, Allocator, Traits>::Swap(int i, int j)
{
    Type temp = this->Get(i);
    this->Set(i) = this->Get(j);
    this->Set(j) = temp;
}

template <class Type, class Allocator, class Traits>
inline int HeapArray<Type, Allocator, Traits>::GetParent(int i)
{
    if (i <= 0)
    {
        return -1;
    }
    else
    {
        return (i - 1) / 2;
    }
}

template <class Type, class Allocator, class Traits>
inline int HeapArray<Type, Allocator, Traits>::GetFirstSon(int i)
{
    int n = 2 * i + 1;
    if (n >= this->Size())
    {
        return -1;
    }
    else
    {
        return n;
    }
}

template <class Type, class Allocator, class Traits>
inline int HeapArray<Type, Allocator, Traits>::GetBrother(int i)
{
    int n = i + 1;
    if (n >= this->Size())
    {
        return -1;
    }
    else
    {
        return n;
    }
}

template <class Type, class Allocator, class Traits>
void HeapArray<Type, Allocator, Traits>::HeapDown(int i)
{
    while (true)
    {
        int n = GetFirstSon(i);
        if (n < 0)
        {
            return;
        }
        int m = GetBrother(n);
        if (m >= 0)
        {
            if (Traits::IsLess(this->Get(m), this->Get(n)))
            {
                n = m;
            }
        }
        if (Traits::IsLessOrEqual(this->Get(i), this->Get(n)))
        {
            return;
        }
        Swap(i, n);
        i = n;
    }
}

template <class Type, class Allocator, class Traits>
void HeapArray<Type, Allocator, Traits>::HeapUp(int i)
{
    while (true)
    {
        int n = GetParent(i);
        if (n < 0)
        {
            return;
        }
        if (Traits::IsLessOrEqual(this->Get(n), this->Get(i)))
        {
            return;
        }
        Swap(n, i);
        i = n;
    }
}

template <class Type, class Allocator, class Traits>
bool HeapArray<Type, Allocator, Traits>::HeapRemoveFirst(Type& item)
{
    int n = this->Size() - 1;
    if (n < 0)
    {
        return false;
    }
    item = this->Get(0);
    Swap(0, n);
    this->Delete(n);
    HeapDown(0);
    return true;
}

template <class Type, class Allocator, class Traits>
void HeapArray<Type, Allocator, Traits>::HeapInsert(const Type& item)
{
    int n = this->Add(item);
    HeapUp(n);
}

template <class Type, class Allocator, class Traits>
void HeapArray<Type, Allocator, Traits>::HeapUpdateUp(const Type& item)
{
    int n = this->Find(item);
    if (n >= 0)
    {
        HeapUp(n);
    }
}

template <class Type, class Allocator, class Traits>
void HeapArray<Type, Allocator, Traits>::HeapDelete(const Type& item)
{
    int n = this->Find(item);
    if (n >= 0)
        HeapDelete(n);
}

template <class Type, class Allocator, class Traits>
void HeapArray<Type, Allocator, Traits>::HeapDelete(int index)
{
    int n = this->Size() - 1;
    if (index > n)
        return;
    Swap(index, n);
    this->Delete(n);

    int parent = GetParent(index);
    if (parent >= 0 && Traits::IsLessOrEqual(this->Get(index), this->Get(parent)))
        HeapUp(index);
    else
        HeapDown(index);
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::AutoArray;
using ::Poseidon::Foundation::FindArray;
using ::Poseidon::Foundation::FindArrayKey;
using ::Poseidon::Foundation::FindArrayKeyTraits;
using ::Poseidon::Foundation::HeapArray;
using ::Poseidon::Foundation::HeapTraits;
using ::Poseidon::Foundation::RefArray;
