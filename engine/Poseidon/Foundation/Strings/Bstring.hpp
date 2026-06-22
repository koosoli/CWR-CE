#pragma once

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/platform.hpp>

// Disable ASan for this function - it falsely detects overlap when there is none
#ifdef __clang__

namespace Poseidon::Foundation
{
__attribute__((no_sanitize("address")))
#elif defined(_MSC_VER) && defined(__SANITIZE_ADDRESS__)
__declspec(no_sanitize_address)
#endif
__forceinline void strcpyLtd(char* dst, const char* src, int size)
{
    strncpy(dst, src, size - 1);
}

__forceinline void strcatLtd(char* dst, const char* src, int size)
{
    while (size > 0 && *dst)
    {
        dst++;
        size--;
    }
    strcpyLtd(dst, src, size);
}

// fixed-size char buffer that guards against overflow
template <int Size>
class BString
{
    char _data[Size];

    // bounded copy from a raw string
    void Copy(const char* src) { strcpyLtd(_data, src, sizeof(_data)); }

  public:
    BString()
    {
        _data[0] = 0;
        // last byte is an overflow sentinel: it must stay zero
        _data[Size - 1] = 0;
    }
    ~BString() { PoseidonAssert(IsValid()); }

    // valid only while the overflow sentinel (last byte) is zero
    bool IsValid() const
    {
        if (_data[sizeof(_data) - 1])
        {
            Fail("BString overflow.");
            return false;
        }
        return true;
    }

    operator const char*() const { return _data; }
    const char* cstr() const { return _data; }

    BString(const char* src) { Copy(src); }
    const BString& operator=(const char* src)
    {
        Copy(src);
        return *this;
    }

    char& operator[](int i)
    {
        PoseidonAssert(i >= 0);
        PoseidonAssert(i < sizeof(_data));
        return _data[i];
    }
    const char& operator[](int i) const
    {
        PoseidonAssert(i >= 0);
        PoseidonAssert(i < sizeof(_data));
        return _data[i];
    }

    // assignment via snprintf — overflow-safe
    const BString& operator=(const BString& src)
    {
        snprintf(_data, sizeof(_data), "%s", (const char*)src._data);
        return *this;
    }
    // assignment via snprintf — overflow-safe
    BString(const BString& src) { snprintf(_data, sizeof(_data), "%s", (const char*)src._data); }
    const BString& operator+=(const char* src)
    {
        strcatLtd(_data, src, sizeof(_data));
        return *this;
    }
    // sprintf into the buffer
    int PrintF(const char* format, ...)
    {
        va_list va;
        va_start(va, format);
        int ret = vsnprintf(_data, sizeof(_data) - 1, format, va);
        va_end(va);
        return ret;
    }
    // vsprintf into the buffer
    int VPrintF(const char* format, va_list va) { return vsnprintf(_data, sizeof(_data) - 1, format, va); }
    // strncpy into the buffer
    const BString& StrNCpy(const char* src, int n)
    {
        if (n < sizeof(_data) - 1)
        {
            ::strncpy(_data, src, n);
        }
        else
        {
            strcpyLtd(_data, src, sizeof(_data));
        }
        return *this;
    }
    BString& StrLwr()
    {
        strlwr(_data);
        return *this;
    }
    BString& StrUpr()
    {
        strupr(_data);
        return *this;
    }
};

// free-function overloads mirroring the C string API for BString
template <int Size>
inline const BString<Size>& strcpy(BString<Size>& dst, const char* src)
{
    return dst = src;
}

template <int Size>
inline const BString<Size>& strncpy(BString<Size>& dst, const char* src, int n)
{
    dst.StrNCpy(src, n);
    return dst;
}

template <int Size>
inline const BString<Size>& strcat(BString<Size>& dst, const char* src)
{
    return dst += src;
}

template <int Size>
inline BString<Size>& strlwr(BString<Size>& dst)
{
    return dst.StrLwr();
}

template <int Size>
inline BString<Size>& strupr(BString<Size>& dst)
{
    return dst.StrUpr();
}

#ifdef _WIN32
template <int Size>
inline int sprintf(BString<Size>& dst, const char* format, ...)
#else
template <int Size>
int sprintf(BString<Size>& dst, const char* format, ...)
#endif
{
    va_list va;
    va_start(va, format);
    int ret = dst.VPrintF(format, va);
    va_end(va);
    return ret;
}

#ifdef _WIN32
template <int Size>
inline int vsprintf(BString<Size>& dst, const char* format, va_list argptr)
#else
template <int Size>
int vsprintf(BString<Size>& dst, const char* format, va_list argptr)
#endif
{
    return dst.VPrintF(format, argptr);
}

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::BString;
