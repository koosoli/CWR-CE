#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/TypeOpts.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <ctype.h>

#ifdef _CPPRTTI
#include <typeinfo>
#endif

#include <Poseidon/Foundation/Containers/Array.hpp>


namespace Poseidon::Foundation
{
const int CoefExpand = 32;

static inline unsigned int CalculateStringHashValue(const char* key, unsigned int hashValue = 0)
{
    while (*key)
    {
        hashValue = hashValue * 33 + (unsigned char)(*key++);
    }
    return hashValue;
}
static inline unsigned int CalculateStringHashValueCI(const char* key, unsigned int hashValue = 0)
{
    while (*key)
    {
        hashValue = hashValue * 33 + (unsigned char)tolower((unsigned char)*key++);
    }
    return hashValue;
}

// Key/hash/compare properties for values stored in MapStringToClass.
template <class Type>
struct MapClassTraits
{
    typedef const char* KeyType;
    static unsigned int CalculateHashValue(KeyType key) { return CalculateStringHashValue(key); }

    // Returns <0 / 0 / >0 for k1 less than / equal to / greater than k2.
    static int CmpKey(KeyType k1, KeyType k2) { return strcmp(k1, k2); }

    static KeyType GetKey(const Type& item) { return item.GetKey(); }
};

// Hashing with chains: inserts may grow the table, removes leave its size unchanged.
// Type must provide GetKey(); Container must provide Size(), MaxSize(), Realloc(int) and
// Add(Type), e.g. Type=RString with Container=AutoArray<RString>.
template <class Type, class Container, class Traits = MapClassTraits<Type>>
class MapStringToClass
{
    typedef typename Traits::KeyType KeyType;

  protected:
    Container* _hashTable;
    int _tableSize; // number of containers (chains)
    int _count;     // number of stored items
    // Meyers singleton for the null sentinel - avoids the static-init order fiasco.
    static Type& GetNull()
    {
        [[clang::no_destroy]] static Type _null;
        return _null;
    }
    enum
    {
        DefTableSize = 15
    };

  public:
    class Iterator
    {
      protected:
        int _table;
        int _item;
        const MapStringToClass<Type, Container, Traits>* _base;

        void FindNextValid()
        {
            while (_table < _base->NTables() && _item >= _base->GetTable(_table).Size())
            {
                _table++;
                _item = 0;
            }
        }

      public:
        Iterator(const MapStringToClass<Type, Container, Traits>& base)
        {
            _table = 0;
            _item = 0;
            _base = &base;
            FindNextValid();
        }
        operator bool() const { return _table < _base->NTables(); }
        void operator++()
        {
            if (_table >= _base->NTables())
            {
                return;
            }
            ++_item;
            FindNextValid();
        }
        const Type& operator*() { return _base->GetTable(_table)[_item]; }
    };

    MapStringToClass(int tableSize = DefTableSize)
    {
        _hashTable = nullptr;
        Init(tableSize);
    }
    ~MapStringToClass() { Clear(); }
    MapStringToClass(const MapStringToClass& src);
    void operator=(const MapStringToClass& src);
    // Free any existing table and set the new size.
    void Init(int tableSize);
    void Clear();
    // Rebuild with a new table size; existing entries are rehashed.
    void Rebuild(int tableSize);

    // Pre-grow the table for nElem elements; speeds up bulk insertion.
    void Reserve(int nElem);

    // Shrink every container to fit once no further changes are expected.
    void Compact();

#if !defined _MSC_VER || _MSC_VER >= 1300
    // Const visit: func(item, this) may not modify items or the map. Return true to stop early.
    template <class Func>
    void ForEachF(Func func) const;

    // Mutating visit: func(item, this) may modify items and the map. Return true to stop early.
    template <class Func>
    void ForEachF(Func func);
#endif

    // Mutating visit via callback; context is forwarded to each call.
    void ForEach(void (*Func)(Type&, MapStringToClass*, void*), void* context = nullptr);

    // Const visit via callback; context is forwarded to each call.
    void ForEach(void (*Func)(const Type&, const MapStringToClass*, void*), void* context = nullptr) const;
    // Get/Set look up by key and return Null() when absent (test with IsNull/NotNull).
    const Type& Get(KeyType key) const;
    Type& Set(KeyType key);
    Type& operator[](KeyType key) { return Set(key); }
    const Type& operator[](KeyType key) const { return Get(key); }
    // Insert value, replacing any entry with the same key; grows the table when overloaded.
    // Returns the container index, or -1 on error.
    int Add(const Type& value);
    // Remove the entry with key; the container count is left unchanged. False if not found.
    bool Remove(KeyType key);
    static bool IsNull(const Type& value) { return &value == &GetNull(); }
    static bool NotNull(const Type& value) { return &value != &GetNull(); }
    static Type& Null() { return GetNull(); }

  public:
    // Direct access - used only for serialization
    int NItems() const { return _count; }
    int NTables() const { return _hashTable ? _tableSize : 0; }
    Container& GetTable(int i) const { return _hashTable[i]; }

  protected:
    // Grow a chain so it can hold one more element (capacity doubles when full).
    static void GrowChainForOne(Container& dest)
    {
        int need = dest.Size() + 1;
        int haveSize = dest.MaxSize();
        if (need > haveSize)
        {
            if (haveSize < 1)
            {
                haveSize = 1;
            }
            while (need > haveSize)
            {
                haveSize = haveSize * 2;
            }
            dest.Realloc(haveSize);
        }
    }
    // Hash key into [0, n); n <= 0 means use _tableSize.
    int HashKey(KeyType key, int n = 0) const;

};

template <class Type, class Container, class Traits>
MapStringToClass<Type, Container, Traits>::MapStringToClass(const MapStringToClass& src)
{
    _count = src._count;
    _tableSize = src._tableSize;
    if (src._hashTable)
    {
        _hashTable = new Container[_tableSize];
        for (int i = 0; i < _tableSize; i++)
        {
            _hashTable[i] = src._hashTable[i];
        }
    }
    else
    {
        _hashTable = nullptr;
    }
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::operator=(const MapStringToClass& src)
{
    Clear();
    _count = src._count;
    _tableSize = src._tableSize;
    if (src._hashTable)
    {
        _hashTable = new Container[_tableSize];
        for (int i = 0; i < _tableSize; i++)
        {
            _hashTable[i] = src._hashTable[i];
        }
    }
    else
    {
        _hashTable = nullptr;
    }
}

template <class Type, class Container, class Traits>
int MapStringToClass<Type, Container, Traits>::HashKey(KeyType key, int n) const
{
    if (n <= 0)
    {
        n = _tableSize;
    }
    unsigned int nHash = Traits::CalculateHashValue(key);
    // well desinged hash function should be different
    // for strings of any length that start with different letters
    // this forces nMash multiplier not equal to power of 2
    // in this case actual nHash multiplier is 33, which is OK
    return nHash % n;
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::Init(int tableSize)
{
    Clear();
    _tableSize = tableSize;
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::Clear()
{
    if (_hashTable)
    {
        delete[] _hashTable;
        _hashTable = nullptr;
    }
    _count = 0;
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::Rebuild(int tableSize)
{
    if (!_hashTable)
    {
        _tableSize = tableSize;
        return;
    }
    Container* newTable = new Container[tableSize];
    for (int i = 0; i < _tableSize; i++)
    {
        const Container& container = _hashTable[i];
        for (int j = 0; j < container.Size(); j++)
        {
            const Type& item = container[j];
            int nHashKey = HashKey(Traits::GetKey(item), tableSize);
            Container& dest = newTable[nHashKey];
            GrowChainForOne(dest);
            dest.Add(item);
        }
    }
    delete[] _hashTable;
    _hashTable = newTable;
    _tableSize = tableSize;
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::ForEach(void (*Func)(Type&, MapStringToClass*, void*), void* context)
{
    if (!_hashTable)
    {
        return;
    }
RestartI:
    int n = _tableSize;
    for (int i = 0; i < n; i++)
    {
        Container& container = _hashTable[i];
    RestartJ:
        int m = container.Size();
        for (int j = 0; j < m; j++)
        {
            Type& item = container[j];
            Func(item, this, context);
            if (_tableSize != n)
            {
                goto RestartI;
            }
            if (container.Size() != m)
            {
                goto RestartJ;
            }
        }
    }
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::ForEach(void (*Func)(const Type&, const MapStringToClass*, void*),
                                                        void* context) const
{
    if (!_hashTable)
    {
        return;
    }
    int n = _tableSize;
    for (int i = 0; i < n; i++)
    {
        const Container& container = _hashTable[i];
        int m = container.Size();
        for (int j = 0; j < m; j++)
        {
            const Type& item = container[j];
            Func(item, this, context);
        }
    }
}

#if !defined _MSC_VER || _MSC_VER >= 1300
template <class Type, class Container, class Traits>
template <class Func>
void MapStringToClass<Type, Container, Traits>::ForEachF(Func func)
{
    if (!_hashTable)
        return;
RestartI:
    int n = _tableSize;
    for (int i = 0; i < n; i++)
    {
        Container& container = _hashTable[i];
    RestartJ:
        int m = container.Size();
        for (int j = 0; j < m; j++)
        {
            Type& item = container[j];
            if (func(item, this))
                return;
            if (_tableSize != n)
                goto RestartI;
            if (container.Size() != m)
                goto RestartJ;
        }
    }
}

template <class Type, class Container, class Traits>
template <class Func>
void MapStringToClass<Type, Container, Traits>::ForEachF(Func func) const
{
    if (!_hashTable)
        return;
    int n = _tableSize;
    for (int i = 0; i < n; i++)
    {
        const Container& container = _hashTable[i];
        int m = container.Size();
        for (int j = 0; j < m; j++)
        {
            const Type& item = container[j];
            if (func(item, this))
                return;
        }
    }
}
#endif

template <class Type, class Container, class Traits>
const Type& MapStringToClass<Type, Container, Traits>::Get(KeyType key) const
{
    if (_count <= 0)
    {
        return Null();
    }
    PoseidonAssert(_hashTable);

    int nHashKey = HashKey(key);
    for (int i = 0; i < _hashTable[nHashKey].Size(); i++)
    {
        const Type& item = _hashTable[nHashKey][i];
        if (Traits::CmpKey(Traits::GetKey(item), key) == 0)
        {
            return item;
        }
    }
    return Null();
}

template <class Type, class Container, class Traits>
Type& MapStringToClass<Type, Container, Traits>::Set(KeyType key)
{
    if (_count <= 0)
    {
        return Null();
    }
    PoseidonAssert(_hashTable);

    int nHashKey = HashKey(key);
    for (int i = 0; i < _hashTable[nHashKey].Size(); i++)
    {
        Type& item = _hashTable[nHashKey][i];
        if (Traits::CmpKey(Traits::GetKey(item), key) == 0)
        {
            return item;
        }
    }
    return Null();
}

template <class Type, class Container, class Traits>
int MapStringToClass<Type, Container, Traits>::Add(const Type& value)
{
    KeyType key = Traits::GetKey(value);
    Type& old = Set(key);
    if (!IsNull(old))
    {
        old = value; // key already present: overwrite in place
        return HashKey(key);
    }
    int maxCount = CoefExpand * _tableSize;
    if (_count + 1 > maxCount)
    {
        int tableSize = _tableSize + 1;
        while (_count + 1 > maxCount)
        {
            tableSize *= 2;
            maxCount = CoefExpand * (tableSize - 1);
        }
        Rebuild(tableSize - 1);
    }
    if (!_hashTable)
    {
        PoseidonAssert(_tableSize > 0);
        if (_tableSize <= 0)
        {
            return -1;
        }
        _hashTable = new Container[_tableSize];
    }
    int nHashKey = HashKey(key);
    Container& dest = _hashTable[nHashKey];
    GrowChainForOne(dest);
    dest.Add(value);
    _count++;
    return nHashKey;
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::Reserve(int nElem)
{
    int maxCount = CoefExpand * _tableSize;
    if (nElem > maxCount)
    {
        int tableSize = _tableSize + 1;
        while (nElem > maxCount)
        {
            tableSize *= 2;
            maxCount = CoefExpand * (tableSize - 1);
        }
        Rebuild(tableSize - 1);
    }
}

template <class Type, class Container, class Traits>
void MapStringToClass<Type, Container, Traits>::Compact()
{
    if (!_hashTable)
    {
        return;
    }
    for (int i = 0; i < _tableSize; i++)
    {
        _hashTable[i].Compact();
    }
}

template <class Type, class Container, class Traits>
bool MapStringToClass<Type, Container, Traits>::Remove(KeyType key)
{
    if (_count <= 0)
    {
        return false;
    }
    PoseidonAssert(_hashTable);

    int nHashKey = HashKey(key);
    for (int i = 0; i < _hashTable[nHashKey].Size(); i++)
    {
        Type& item = _hashTable[nHashKey][i];
        if (Traits::CmpKey(Traits::GetKey(item), key) == 0)
        {
            _hashTable[nHashKey].Delete(i, 1);
            _count--;
            PoseidonAssert(_count >= 0);
            if (_count == 0)
            {
                Clear();
            }
            return true;
        }
    }
    return false;
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::MapClassTraits;
using ::Poseidon::Foundation::MapStringToClass;
using ::Poseidon::Foundation::CalculateStringHashValue;
using ::Poseidon::Foundation::CalculateStringHashValueCI;
