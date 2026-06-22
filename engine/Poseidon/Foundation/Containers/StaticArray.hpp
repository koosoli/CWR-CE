#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>


namespace Poseidon::Foundation
{
template <class Type>
class StaticArray : public AutoArray<Type, Poseidon::Foundation::MemAllocSS>
{
  public:
    StaticArray() = default;
    explicit StaticArray(const Poseidon::Foundation::MemAllocSS& alloc) { this->SetStorage(alloc); }
};

#define StaticStorage MStorage

template <class Type>
class StaticArrayAuto : public AutoArray<Type, Poseidon::Foundation::MemAllocSA>
{
  public:
    StaticArrayAuto() = default; // no init - use dynamic allocation
    explicit StaticArrayAuto(void* mem, int size) { this->_alloc.Init(mem, size); }
};

#define AUTO_STORAGE_ALIGNED(Type, name, size, atype)                      \
    const int name##_minGrow = StaticArrayAuto<Type>::MinGrow;             \
    const int name##_size = size > name##_minGrow ? size : name##_minGrow; \
    atype name[(name##_size * sizeof(Type) + sizeof(atype) - 1) / sizeof(atype)];

#define AUTO_STATIC_ARRAY_ALIGNED(Type, name, size, atype)                                  \
    const int name##_minGrow = StaticArrayAuto<Type>::MinGrow;                              \
    const int name##_size = size > name##_minGrow ? size : name##_minGrow;                  \
    atype name##_storage[(name##_size * sizeof(Type) + sizeof(atype) - 1) / sizeof(atype)]; \
    StaticArrayAuto<Type> name(name##_storage, sizeof(name##_storage));                     \
    name.Realloc(name##_size);

#define AUTO_STATIC_ARRAY_1(Type, name, size) AUTO_STATIC_ARRAY_ALIGNED(Type, name, size, char)

#define AUTO_STATIC_ARRAY_2(Type, name, size) AUTO_STATIC_ARRAY_ALIGNED(Type, name, size, short)

#define AUTO_STATIC_ARRAY_4(Type, name, size) AUTO_STATIC_ARRAY_ALIGNED(Type, name, size, int)

#define AUTO_STATIC_ARRAY_8(Type, name, size) AUTO_STATIC_ARRAY_ALIGNED(Type, name, size, double)

#ifdef _KNI
#define AUTO_STATIC_ARRAY_16(Type, name, size) AUTO_STATIC_ARRAY_ALIGNED(Type, name, size, __m128)

#define AUTO_STATIC_ARRAY AUTO_STATIC_ARRAY_16
#else
#define AUTO_STATIC_ARRAY AUTO_STATIC_ARRAY_8
#endif

#define AUTO_FIND_ARRAY_ALIGNED(Type, name, size, atype)                                    \
    const int name##_minGrow = FindArray<Type, Poseidon::Foundation::MemAllocSA>::MinGrow;                        \
    const int name##_size = size > name##_minGrow ? size : name##_minGrow;                  \
    atype name##_storage[(name##_size * sizeof(Type) + sizeof(atype) - 1) / sizeof(atype)]; \
    Poseidon::Foundation::MemAllocSA name##_alloc;                                          \
    name##_alloc.Init(name##_storage, sizeof(name##_storage));                              \
    FindArray<Type, Poseidon::Foundation::MemAllocSA> name;                                 \
    name.SetStorage(name##_alloc);                                                          \
    name.Realloc(name##_size);

#define AUTO_FIND_ARRAY_1(Type, name, size) AUTO_FIND_ARRAY_ALIGNED(Type, name, size, char)

#define AUTO_FIND_ARRAY_2(Type, name, size) AUTO_FIND_ARRAY_ALIGNED(Type, name, size, short)

#define AUTO_FIND_ARRAY_4(Type, name, size) AUTO_FIND_ARRAY_ALIGNED(Type, name, size, int)

#define AUTO_FIND_ARRAY_8(Type, name, size) AUTO_FIND_ARRAY_ALIGNED(Type, name, size, double)

#ifdef _KNI
#define AUTO_FIND_ARRAY_16(Type, name, size) AUTO_FIND_ARRAY_ALIGNED(Type, name, size, __m128)

#define AUTO_FIND_ARRAY AUTO_FIND_ARRAY_16
#else
#define AUTO_FIND_ARRAY AUTO_FIND_ARRAY_8
#endif

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::StaticArray;
using ::Poseidon::Foundation::StaticArrayAuto;
