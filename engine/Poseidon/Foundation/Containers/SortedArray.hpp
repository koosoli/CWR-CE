#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Algorithms/BSearch.hpp>
#include <Poseidon/Foundation/Algorithms/Qsort.hpp>


namespace Poseidon::Foundation
{
template <class Type>
struct SortedArrayTraits
{
    typedef const Type& KeyType;

    static KeyType GetKey(const Type& type) { return type; }
    static int Compare(KeyType a, KeyType b)
    {
        if (a > b)
            return +1;
        if (a < b)
            return -1;
        return 0;
    }
};

template <class Type, class Allocator = MemAllocD, class Traits = SortedArrayTraits<Type>>
class SortedArray : private AutoArray<Type, Allocator>
{
    typedef typename Traits::KeyType KeyType;
    typedef AutoArray<Type, Allocator> base;

  public:
    SortedArray();

    // AddDirty appends unsorted; call Sort() before any search-based op
    // (Add, AddUnique, Find, FindKey, Delete, DeleteKey).
    void AddDirty(const Type& src) { base::Add(src); }

    void Add(const Type& src);
    // Add only if no item with the same key is already present.
    bool AddUnique(const Type& src)
    {
        if (Find(src) >= 0)
            return false;
        Add(src);
        return true;
    }
    // Replace the value at index; the new value must share its key.
    void Replace(int index, const Type& src)
    {
        PoseidonAssert(Traits::Compare(Traits::GetKey(Get(index)), Traits::GetKey(src)));
        this->_data[index] = src;
    }
    int Find(const Type& src) const { return FindKey(Traits::GetKey(src)); }
    int FindKey(KeyType key) const;

    bool Delete(const Type& src) { return DeleteKey(Traits::GetKey(src)); }
    bool DeleteKey(KeyType src);

    // Curated re-export of the privately inherited AutoArray interface.
    __forceinline const Type* Data() const { return base::Data(); }
    __forceinline int Size() const { return base::Size(); }
    __forceinline const Type& operator[](int i) const { return base::Get(i); }
    __forceinline void Compact() { base::Compact(); }
    __forceinline void Clear() { base::Clear(); }
    __forceinline void Realloc(int sizeNeed, int sizeWant) { base::Realloc(sizeNeed, sizeWant); }
    __forceinline void Realloc(int size) { base::Realloc(size); }
    __forceinline void Reserve(int sizeNeed, int sizeWant) { base::Reserve(sizeNeed, sizeWant); }

    // Re-establish sorted order after a batch of AddDirty calls.
    void Sort();

};

template <class Type, class Allocator, class Traits>
SortedArray<Type, Allocator, Traits>::SortedArray()
{
}

template <class Type, class Allocator, class Traits>
bool SortedArray<Type, Allocator, Traits>::DeleteKey(KeyType key)
{
    int index = FindKey(key);
    if (index < 0)
        return false;
    base::Delete(index);
    return true;
}

// Functor passed to QSort.
template <class Type, class Traits>
struct SortedArrayCompare
{
    int operator()(const Type* a, const Type* b, void*)
    {
        return Traits::Compare(Traits::GetKey(*a), Traits::GetKey(*b));
    }
};

template <class Type, class Traits = SortedArrayTraits<Type>>
struct CompareWithKey
{
    int operator()(Traits::KeyType b, const Type& a) { return Traits::Compare(b, Traits::GetKey(a)); }
};

template <class Type, class Allocator, class Traits>
void SortedArray<Type, Allocator, Traits>::Add(const Type& src)
{
    KeyType key = Traits::GetKey(src);
    int index = BSearchPos(Data(), Size(), CompareWithKey<Type, Traits>(), key);
#if _DEBUG
    PoseidonAssert(index <= Size());
    PoseidonAssert(index >= 0);
    if (index == Size())
    {
        PoseidonAssert(Size() == 0 || Traits::Compare(key, Traits::GetKey(Get(Size() - 1))) > 0);
    }
    else
    {
        PoseidonAssert(Traits::Compare(key, Traits::GetKey(Get(index))) <= 0);
        PoseidonAssert(index <= 0 || Traits::Compare(key, Traits::GetKey(Get(index - 1))) >= 0);
    }
#endif
    base::Insert(index, src);
}

template <class Type, class Allocator, class Traits>
void SortedArray<Type, Allocator, Traits>::Sort()
{
    SortedArrayCompare<Type, Traits> compare;
    QSortWithContext(base::Data(), Size(), (void*)nullptr, compare);
}

template <class Type, class Allocator, class Traits>
int SortedArray<Type, Allocator, Traits>::FindKey(KeyType key) const
{
    return BSearch(Data(), Size(), CompareWithKey<Type, Traits>(), key);
    return -1;
}

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::SortedArray;
using ::Poseidon::Foundation::SortedArrayTraits;
