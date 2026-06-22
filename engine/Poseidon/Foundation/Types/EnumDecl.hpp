#pragma once

#include <Poseidon/Foundation/platform.hpp>

#if _MSC_VER && !NO_ENUM_FORWARD_DECLARATION

// Microsoft compiler supports enum forward declaration
#define DECL_ENUM(Name) enum Name;

#define DEFINE_ENUM_BEG(Name) \
    enum Name                 \
    {
#define DEFINE_ENUM_END(Name) \
    }                         \
    ;

#elif !NO_UNDEF_ENUM_REFERENCE

#include <limits.h>

// workaround - forward declare using class enum_base

class enum_base
{
    int _val;

  public:
    operator int() const { return _val; }
    explicit enum_base(int val) { _val = val; }
    enum_base() {}
};

#define DECL_ENUM(Name)                       \
    class Name : public enum_base             \
    {                                         \
      public:                                 \
        Name() : enum_base() {}               \
        Name(int val) : enum_base(val) {}     \
        Name(const Name& a) : enum_base(a) {} \
    };

#define DEFINE_ENUM_BEG(Name) \
    enum Name##_Enum          \
    {
#define DEFINE_ENUM_END(Name) \
    }                         \
    ;

#else

#include <limits.h>

// workaround - forward declare using class enum_base

class enum_base
{
    int _val;

  public:
    operator int() const { return _val; }
    explicit enum_base(int val) { _val = val; }
    enum_base() {}
};

// declare enum - may be used only once
#define DECL_ENUM(Name)                                     \
    class ClassHelperEnum_##Name;                           \
    class Name : public enum_base                           \
    {                                                       \
      public:                                               \
        Name() : enum_base() {}                             \
        explicit Name(int val) : enum_base(val) {}          \
        Name(const Name& a) : enum_base(a) {}               \
        template <class Type>                               \
        Name(Type x) : enum_base(ClassHelperEnum_##Name(x)) \
        {                                                   \
        }                                                   \
    };

// define an enum and its values; DECL_ENUM must precede this
#define DEFINE_ENUM_BEG(Name) \
    enum Name##_Enum          \
    {
#define DEFINE_ENUM_END(Name)                                       \
    }                                                               \
    ;                                                               \
    class ClassHelperEnum_##Name : public enum_base                 \
    {                                                               \
      public:                                                       \
        ClassHelperEnum_##Name() {}                                 \
        ClassHelperEnum_##Name(Name##_Enum val) : enum_base(val) {} \
    };

#if REMARK_EXAMPLE
// typical usage example
DECL_ENUM(SomeEnum)
DEFINE_ENUM_BEG(SomeEnum)
SomeEnumValue0, SomeEnumValue1, SomeEnumValue2, SomeEnumValue3, DEFINE_ENUM_END(SomeEnum)
#endif

#endif // _MSC_VER && !NO_ENUM_FORWARD_DECLARATION

