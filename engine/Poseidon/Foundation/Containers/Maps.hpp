#pragma once

#include <Poseidon/Foundation/Containers/ConstructTraitsModern.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Memory/MemAlloc.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Types/ScopeLock.hpp>

namespace Poseidon::Foundation
{
// Iterator state for map passes.
typedef unsigned IteratorState;

// Sentinel iterator state marking the end of a pass.
const IteratorState ITERATOR_NULL = 0xffffffff;

// Default hash-array load factor.
const float DEFAULT_LOAD_FACTOR = 0.5f;

// Below this many removed (zombie) slots the table is not reorganized.
const unsigned REMOVED_LOW_MARK = 6;

// Widest signed integer key type.
typedef int64 SuperInt;

// Smallest pre-tabulated prime >= s, used as the hash-array size.
extern unsigned hashSize(unsigned s);

// null/zombie sentinels for an implicit map's TargetType.
template <typename TargetType>
struct ImplicitMapTraits
{
    static TargetType null;   // empty slot
    static TargetType zombie; // removed slot (internal)
};

template <typename TargetType>
TargetType ImplicitMapTraits<TargetType>::null;

template <typename TargetType>
TargetType ImplicitMapTraits<TargetType>::zombie;

// null/keyNull/zombie sentinels for an explicit map.
template <typename KeyType, typename TargetType>
struct ExplicitMapTraits
{
    static TargetType null; // empty target slot
    static KeyType keyNull; // empty key slot (internal)
    static KeyType zombie;  // removed key (internal)
    // Default hash: reinterpret the key as unsigned.
    static unsigned getHash(KeyType& key) { return (unsigned)key; }
};

template <typename KeyType, typename TargetType>
TargetType ExplicitMapTraits<KeyType, TargetType>::null;

template <typename KeyType, typename TargetType>
KeyType ExplicitMapTraits<KeyType, TargetType>::keyNull;

template <typename KeyType, typename TargetType>
KeyType ExplicitMapTraits<KeyType, TargetType>::zombie;

#ifdef MAP_STAT

#define IncResize statResize++
#define IncGet statGet++
#define IncPut statPut++
#define IncReplace statReplace++
#define IncRemove statRemove++
#define IncZombie statZombie++
#define IncCollision statCollision++

#else

#define IncResize
#define IncGet
#define IncPut
#define IncReplace
#define IncRemove
#define IncZombie
#define IncCollision

#endif

// Open-addressing hash map from a numeric key to TargetType, where each value's key is
// derived in place by the implicitKey() function (no separate key array). TargetType needs
// operator= and operator==; KeyType needs operator==. See ExplicitMap for stored keys.
template <typename KeyType, typename TargetType, KeyType implicitKey(TargetType&), bool mtSafe = false,
          typename Allocator = MemAllocD>
class ImplicitMap
{
  protected:
    typedef ImplicitMapTraits<TargetType> Traits;

    typedef Poseidon::Foundation::ModernTraits<TargetType> CTraits;

    unsigned used;      // number of live pairs
    unsigned allocated; // allocated slots
    unsigned removed;   // zombie slots

    // Reasonable range: 0.25 (fast, sparse) to 0.8 (slow, compact).
    float loadFactor;

    unsigned overLoad; // resize threshold

    AutoArray<TargetType, Allocator> table;

    PoCriticalSection lock; // guards the map when mtSafe

    inline void enter()
    {
        if (mtSafe)
        {
            lock.enter();
        }
    }

    inline void leave()
    {
        if (mtSafe)
        {
            lock.leave();
        }
    }

    // Resize/reorganize to hold at least newSize pairs. Caller must hold the lock.
    void resize(unsigned newSize)
    {
        if (newSize < used)
        {
            return; // newSize == used => reorganize the table
        }
        IncResize;
#ifdef NET_LOG_MAPS
        NetLog("ImplicitMap::resize: newSize=%u(x %u), used=%u, removed=%u, allocated=%u, overload=%u", newSize,
               sizeof(TargetType), used, removed, allocated, overLoad);
#endif
        unsigned newAllocated = hashSize((unsigned)(newSize / loadFactor + 1.0f));
        AutoArray<TargetType, Allocator> newTable(newAllocated);
        unsigned i, j;
        for (i = 0; i++ < newAllocated;)
        {
            newTable.AddFast(Traits::null);
        }
        for (j = 0; j < allocated; j++)
        {
            if (table[j] != Traits::null && table[j] != Traits::zombie)
            {
                IncPut;
                i = (unsigned)(implicitKey(table[j]) % newAllocated);
                while (newTable[i] != Traits::null)
                {
                    IncCollision;
                    if (++i >= newAllocated)
                    {
                        i = 0;
                    }
                }
                CTraits::Destruct(newTable[i]);
                CTraits::MoveData(&newTable[i], &table[j], 1);
            }
            else
            {
                CTraits::Destruct(table[j]);
            }
        }
        if (table.Size())
        {
#ifdef NET_LOG_ZOMBIE
            int zombies = 0;
            for (i = 0; i < allocated;)
                if (table[i++] == Traits::zombie)
                    zombies++;
            NetLog("ImplicitMap::resize: destroying zombies: pred count=%d, number=%d", Traits::zombie->RefCounter(),
                   zombies);
#endif
        }
        table.UnsafeResize(0); // all instances were moved to newTable
        table.Move(newTable);
        allocated = newAllocated;
        overLoad = (unsigned)(allocated * loadFactor) + 1;
        removed = 0;
    }

  public:
    // External access to the null sentinel.
    static inline const TargetType& null() { return Traits::null; }

#ifdef MAP_STAT
    unsigned statResize;
    unsigned statGet;
    unsigned statPut;
    unsigned statReplace;
    unsigned statRemove;
    unsigned statZombie;
    unsigned statCollision;
#endif

    // Optionally pre-allocate initSize pairs. initLoadFactor in [0.25, 0.8].
    ImplicitMap(unsigned initSize = 0, float initLoadFactor = DEFAULT_LOAD_FACTOR)
        : LockInit(lock, "ImplicitMap", mtSafe)
    {
        enter();
        used = removed = allocated = overLoad = 0;
        loadFactor = initLoadFactor;
#ifdef MAP_STAT
        statResize = statGet = statPut = statReplace = statRemove = statZombie = statCollision = 0;
#endif
        if (initSize > 0)
        {
            resize(initSize);
        }
        leave();
    }

    // Find key; sets result to the value (or null) and returns whether it was found.
    bool get(KeyType key, TargetType& result)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncGet;
        if (used > 0)
        {
            unsigned i = (unsigned)(key % allocated);
            while (table[i] != Traits::null)
            {
                if (table[i] != Traits::zombie && implicitKey(table[i]) == key)
                {
                    result = table[i];
                    return true;
                }
                IncCollision;
                if (++i >= allocated)
                {
                    i = 0;
                }
            }
        }
        result = Traits::null;
        return false;
    }

    // Find key; returns a pointer to the value, or nullptr if absent.
    TargetType* get(KeyType key)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncGet;
        if (used > 0)
        {
            unsigned i = (unsigned)(key % allocated);
            while (table[i] != Traits::null)
            {
                if (table[i] != Traits::zombie && implicitKey(table[i]) == key)
                {
                    return (&table[i]);
                }
                IncCollision;
                if (++i >= allocated)
                {
                    i = 0;
                }
            }
        }
        return nullptr;
    }

    // True if key is present.
    bool present(KeyType key)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncGet;
        if (used > 0)
        {
            unsigned i = (unsigned)(key % allocated);
            while (table[i] != Traits::null)
            {
                if (table[i] != Traits::zombie && implicitKey(table[i]) == key)
                {
                    return true;
                }
                IncCollision;
                if (++i >= allocated)
                {
                    i = 0;
                }
            }
        }
        return false;
    }

    // True if value (by its implicit key) is present.
    bool present(TargetType value)
    {
        if (value == Traits::null || value == Traits::zombie)
        {
            return false;
        }
        return present(implicitKey(value));
    }

    // Insert value (its key is implicit). old, if non-null, receives the replaced value.
    // Returns true when it replaced an existing entry.
    bool put(TargetType value, TargetType* old = nullptr)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        if (value == Traits::null || value == Traits::zombie)
        {
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        IncPut;
        if (!allocated)
        {
            resize(2); // map would have been empty!
        }
        KeyType key = implicitKey(value);
        unsigned i = (unsigned)(key % allocated);
        unsigned insert = allocated;
        while (table[i] != Traits::null)
        {
            if (table[i] == Traits::zombie)
            { // I'll replace this zombie later
                insert = i;
            }
            else if (implicitKey(table[i]) == key)
            { // replace the old item
                IncReplace;
                if (old)
                {
                    *old = table[i];
                }
                table[i] = value;
                return true;
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        if (insert < allocated)
        { // replace the zombie item
            IncReplace;
            used++;
            removed--;
#ifdef NET_LOG_ZOMBIE
            NetLog("ImplicitMap::put: - replacing zombie: pred count=%d", Traits::zombie->RefCounter());
#endif
            table[insert] = value;
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        // insert as a new item:
        if (used + removed + 1 >= overLoad)
        { // I must do resize/reorganization
            if (removed)
            { // prediction of future "remove()"s
                if (removed < REMOVED_LOW_MARK)
                {
                    resize(used + REMOVED_LOW_MARK);
                }
                else
                {
                    resize(used + (removed >> 1) + 1);
                }
            }
            else
            {
                resize(used + 1);
            }
            i = (unsigned)(key % allocated);
            while (table[i] != Traits::null)
            { // looking for an empty slot
                IncCollision;
                if (++i >= allocated)
                {
                    i = 0;
                }
            }
        }
        used++;
        table[i] = value;
        if (old)
        {
            *old = Traits::null;
        }
        return false;
    }

    // Remove the entry for key; old (if non-null) receives it. Returns true if removed.
    bool remove(KeyType key, TargetType* old = nullptr)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncRemove;
        if (used == 0)
        {
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        unsigned i = (unsigned)(key % allocated);
        while (table[i] != Traits::null)
        {
            if (table[i] != Traits::zombie && implicitKey(table[i]) == key)
            { // found!
                if (old)
                {
                    *old = table[i];
                }
#ifdef NET_LOG_DESTRUCTOR
                NetLog("ImplicitMap::remove: - assigning zombie: pred count=%d", Traits::zombie->RefCounter());
#endif
                table[i] = Traits::zombie;
                used--;
                removed++;
                IncZombie;
                return true;
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        if (old)
        {
            *old = Traits::null;
        }
        return false;
    }

    // Remove value by its implicit key. Returns true if removed.
    bool remove(TargetType value)
    {
        if (value == Traits::null || value == Traits::zombie)
        {
            return false;
        }
        return remove(implicitKey(value));
    }

    // Number of live pairs.
    inline unsigned card() const
    {
        return used; // mt-safe (used contains valid value all the time..)
    }

    // Drop all pairs but keep the hash-array allocated.
    void reset()
    {
        enter();
        if (used > 0)
        {
            unsigned i;
            used = removed = 0;
            for (i = 0; i < allocated;)
            {
                table[i++] = Traits::null;
            }
        }
        leave();
    }

    // Begin a pass; iteration order is unspecified. For a cyclic pass, capture
    // origin = iterator after this call. Sets first; returns true if the map is non-empty.
    bool getFirst(IteratorState& iterator, TargetType& first)
    {
        iterator = 0;
        if (used)
        {
            return getNext(iterator, first);
        }
        iterator = ITERATOR_NULL;
        first = Traits::null;
        return false;
    }

    // Advance the pass started by getFirst(); sets next and returns true while items remain.
    bool getNext(IteratorState& iterator, TargetType& next)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        while (iterator < allocated)
        {
            if (table[iterator] != Traits::null && table[iterator] != Traits::zombie)
            {
                next = table[iterator++];
                return true;
            }
            iterator++;
        }
        iterator = ITERATOR_NULL;
        next = Traits::null;
        return false;
    }

    // Begin a cyclic pass from origin (kept externally by the caller). Sets first;
    // returns true if the map is non-empty.
    bool getFirstCyclic(IteratorState& iterator, IteratorState& origin, TargetType& first)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        if (!used)
        {
            first = Traits::null;
            return false;
        }
        if (origin > allocated)
        {
            origin = allocated;
        }
        iterator = origin;
        if (iterator < allocated && table[iterator] != Traits::null && table[iterator] != Traits::zombie)
        {
            first = table[iterator++];
            return true;
        }
        iterator++;
        return getNextCyclic(iterator, origin, first);
    }

    // Advance a cyclic pass started by getFirstCyclic() (origin kept externally); almost
    // thread-safe. Sets next and returns true while items remain.
    bool getNextCyclic(IteratorState& iterator, IteratorState& origin, TargetType& next)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        if (!used || iterator == origin)
        {
            next = Traits::null;
            return false;
        }
        if (origin > allocated)
        {
            origin = allocated;
        }
        if (iterator > origin)
        { // pass to the end of the array
            while (iterator < allocated)
            {
                if (table[iterator] != Traits::null && table[iterator] != Traits::zombie)
                {
                    next = table[iterator++];
                    return true;
                }
                iterator++;
            }
            iterator = 0;
        }
        // pass from the beginning to the origin
        while (iterator < origin)
        {
            if (table[iterator] != Traits::null && table[iterator] != Traits::zombie)
            {
                next = table[iterator++];
                return true;
            }
            iterator++;
        }
        next = Traits::null;
        return false;
    }

    ~ImplicitMap()
    {
        reset();
        enter();
        table.Clear();
        allocated = overLoad = 0; // to be sure
        leave();
    }
};

// Open-addressing hash map from KeyType to TargetType, storing keys explicitly in a
// parallel array. Both KeyType and TargetType need operator= and operator==. See ImplicitMap
// for the variant where the key is derived from the value.
template <typename KeyType, typename TargetType, bool mtSafe = false, typename Allocator = MemAllocD>
class ExplicitMap
{
  protected:
    typedef ExplicitMapTraits<KeyType, TargetType> Traits;

    typedef Poseidon::Foundation::ModernTraits<TargetType> CTraits;
    typedef Poseidon::Foundation::ModernTraits<KeyType> KCTraits;

    unsigned used;      // number of live pairs
    unsigned allocated; // allocated slots
    unsigned removed;   // zombie slots

    // Reasonable range: 0.25 (fast, sparse) to 0.8 (slow, compact).
    float loadFactor;

    unsigned overLoad; // resize threshold

    AutoArray<TargetType, Allocator> table;
    AutoArray<KeyType, Allocator> keys; // parallel to table

    PoCriticalSection lock; // guards the map when mtSafe

    inline void enter()
    {
        if (mtSafe)
        {
            lock.enter();
        }
    }

    inline void leave()
    {
        if (mtSafe)
        {
            lock.leave();
        }
    }

    // Resize/reorganize to hold at least newSize pairs. Caller must hold the lock.
    void resize(unsigned newSize)
    {
        if (newSize < used)
        {
            return; // newSize == used => reorganize the table
        }
        IncResize;
#ifdef NET_LOG_MAPS
        NetLog("ExplicitMap::resize: newSize=%u(x %u+%u), used=%u, removed=%u, allocated=%u, overload=%u", newSize,
               sizeof(KeyType), sizeof(TargetType), used, removed, allocated, overLoad);
#endif
        unsigned newAllocated = hashSize((unsigned)(newSize / loadFactor + 1.0f));
        AutoArray<KeyType, Allocator> newKeys(newAllocated);
        AutoArray<TargetType, Allocator> newTable(newAllocated);
        unsigned i, j;
        for (i = 0; i++ < newAllocated;)
        {
            newKeys.AddFast(Traits::keyNull);
            newTable.AddFast(Traits::null);
        }
        for (j = 0; j < allocated; j++)
        {
            if (keys[j] != Traits::keyNull && keys[j] != Traits::zombie)
            {
                IncPut;
                i = Traits::getHash(keys[j]) % newAllocated;
                while (newKeys[i] != Traits::keyNull)
                {
                    IncCollision;
                    if (++i >= newAllocated)
                    {
                        i = 0;
                    }
                }
                KCTraits::Destruct(newKeys[i]);
                KCTraits::MoveData(&newKeys[i], &keys[j], 1);
                CTraits::Destruct(newTable[i]);
                CTraits::MoveData(&newTable[i], &table[j], 1);
            }
            else
            {
                KCTraits::Destruct(keys[j]);
                CTraits::Destruct(table[j]);
            }
        }
        keys.UnsafeResize(0); // all instances were moved to newKeys
        keys.Move(newKeys);
        table.UnsafeResize(0); // all instances were moved to newTable
        table.Move(newTable);
        allocated = newAllocated;
        overLoad = (unsigned)(allocated * loadFactor) + 1;
        removed = 0;
    }

  public:
    // Verify every stored key is reachable via get(); returns false on the first miss.
    bool checkIntegrity()
    {
        IteratorState it;
        TargetType value;
        KeyType key;
        if (getFirst(it, value, &key))
        {
            do
            {
                TargetType check;
                if (!get(key, check))
                {
                    Fail("get failed");
                    return false;
                }
            } while (getNext(it, value, &key));
        }
        return true;
    }

    // External access to the null sentinel.
    static inline const TargetType& null() { return Traits::null; }

#ifdef MAP_STAT
    unsigned statResize;
    unsigned statGet;
    unsigned statPut;
    unsigned statReplace;
    unsigned statRemove;
    unsigned statZombie;
    unsigned statCollision;
#endif

    // Optionally pre-allocate initSize pairs. initLoadFactor in [0.25, 0.8].
    ExplicitMap(unsigned initSize = 0, float initLoadFactor = DEFAULT_LOAD_FACTOR)
        : LockInit(lock, "ExplicitMap", mtSafe)
    {
        enter();
        used = removed = allocated = overLoad = 0;
        loadFactor = initLoadFactor;
#ifdef MAP_STAT
        statResize = statGet = statPut = statReplace = statRemove = statZombie = statCollision = 0;
#endif
        if (initSize > 0)
        {
            resize(initSize);
        }
        leave();
    }

    // Find key; sets result to the value (or null) and returns whether it was found.
    bool get(KeyType key, TargetType& result)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncGet;
        if (used == 0 || key == Traits::keyNull || key == Traits::zombie)
        {
            result = Traits::null;
            return false;
        }
        unsigned i = Traits::getHash(key) % allocated;
        while (keys[i] != Traits::keyNull)
        {
            if (keys[i] == key)
            {
                result = table[i];
                return true;
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        result = Traits::null;
        return false;
    }

    // Find key; returns a pointer to the value, or nullptr if absent.
    // Dangerous: the map must stay locked while the pointer is in use.
    TargetType* get(KeyType key)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncGet;
        if (used == 0 || key == Traits::keyNull || key == Traits::zombie)
        {
            return nullptr;
        }
        unsigned i = Traits::getHash(key) % allocated;
        while (keys[i] != Traits::keyNull)
        {
            if (keys[i] == key)
            {
                return &table[i];
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        return nullptr;
    }

    // True if key is present.
    bool present(KeyType key)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        IncGet;
        if (used == 0 || key == Traits::keyNull || key == Traits::zombie)
        {
            return false;
        }
        unsigned i = Traits::getHash(key) % allocated;
        while (keys[i] != Traits::keyNull)
        {
            if (keys[i] == key)
            {
                return true;
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        return false;
    }

    // Insert [key, value]. old, if non-null, receives the replaced value.
    // Returns true when it replaced an existing entry.
    bool put(KeyType key, TargetType value, TargetType* old = nullptr)
    {
        if (value == Traits::null)
        {
            return remove(key, old);
        }
        if (key == Traits::keyNull || key == Traits::zombie)
        {
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        ScopeLock<PoCriticalSection> sl(lock);
        IncPut;
        if (!allocated)
        {
            resize(2); // map would have been empty!
        }
        unsigned i = Traits::getHash(key) % allocated;
        unsigned insert = allocated;
        while (keys[i] != Traits::keyNull)
        {
            if (keys[i] == Traits::zombie)
            { // I'll replace this zombie later
                insert = i;
            }
            else if (keys[i] == key)
            { // replace the old item
                IncReplace;
                if (old)
                {
                    *old = table[i];
                }
                table[i] = value;
                return true;
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        if (insert < allocated)
        { // replace the zombie item
            IncReplace;
            used++;
            removed--;
            keys[insert] = key;
            table[insert] = value;
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        // insert as a new item:
        if (used + removed + 1 >= overLoad)
        { // I must do resize/reorganization
            if (removed)
            { // prediction of future "remove()"s
                if (removed < REMOVED_LOW_MARK)
                {
                    resize(used + REMOVED_LOW_MARK);
                }
                else
                {
                    resize(used + (removed >> 1) + 1);
                }
            }
            else
            {
                resize(used + 1);
            }
            i = Traits::getHash(key) % allocated;
            while (keys[i] != Traits::keyNull)
            {
                IncCollision;
                if (++i >= allocated)
                {
                    i = 0;
                }
            }
        }
        used++;
        keys[i] = key;
        table[i] = value;
        if (old)
        {
            *old = Traits::null;
        }
        return false;
    }

    // Remove the entry for key; old (if non-null) receives it. Returns true if removed.
    bool remove(KeyType key, TargetType* old = nullptr)
    {
        if (key == Traits::keyNull || key == Traits::zombie)
        {
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        ScopeLock<PoCriticalSection> sl(lock);
        IncRemove;
        if (used == 0)
        {
            if (old)
            {
                *old = Traits::null;
            }
            return false;
        }
        unsigned i = Traits::getHash(key) % allocated;
        while (keys[i] != Traits::keyNull)
        {
            if (keys[i] == key)
            { // found!
                if (old)
                {
                    *old = table[i];
                }
                keys[i] = Traits::zombie;
                table[i] = Traits::null;
                used--;
                removed++;
                IncZombie;
                return true;
            }
            IncCollision;
            if (++i >= allocated)
            {
                i = 0;
            }
        }
        if (old)
        {
            *old = Traits::null;
        }
        return false;
    }

    // Number of live pairs.
    inline unsigned card() const
    {
        return used; // MT-safe as "used" is always valid.
    }

    // Drop all pairs but keep the hash-arrays allocated.
    void reset()
    {
        enter();
        if (used > 0)
        {
            unsigned i;
            for (i = 0; i < allocated;)
            {
                keys[i] = Traits::keyNull;
                table[i++] = Traits::null;
            }
            used = removed = 0;
        }
        leave();
    }

    // Begin a pass; iteration order is unspecified. Sets first (and *key if key is non-null);
    // returns true if the map is non-empty.
    bool getFirst(IteratorState& iterator, TargetType& first, KeyType* key = nullptr)
    {
        iterator = 0;
        return getNext(iterator, first, key);
    }

    // Advance the pass started by getFirst(); sets next (and *key if key is non-null) and
    // returns true while items remain.
    bool getNext(IteratorState& iterator, TargetType& next, KeyType* key = nullptr)
    {
        ScopeLock<PoCriticalSection> sl(lock);
        while (iterator < allocated)
        {
            if (keys[iterator] != Traits::keyNull && keys[iterator] != Traits::zombie)
            {
                if (key)
                {
                    *key = keys[iterator];
                }
                next = table[iterator++];
                return true;
            }
            iterator++;
        }
        iterator = ITERATOR_NULL;
        if (key)
        {
            *key = Traits::keyNull;
        }
        next = Traits::null;
        return false;
    }

    ~ExplicitMap()
    {
        reset();
        enter();
        keys.Clear();
        table.Clear();
        allocated = overLoad = 0; // to be sure
        leave();
    }
};

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::ImplicitMap;
using ::Poseidon::Foundation::ImplicitMapTraits;
