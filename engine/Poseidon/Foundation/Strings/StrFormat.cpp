#include <Poseidon/Foundation/Strings/StrFormat.hpp>
#include <stdarg.h>
#include <stdio.h>
#include <Poseidon/Foundation/Strings/RString.hpp>


namespace Poseidon::Foundation
{
RString FormatNumber(int number)
{
    char buf[256];
    snprintf(buf, sizeof(buf) - 1, "%d", number);
    buf[sizeof(buf) - 1] = 0;
    return buf;
}

RString Format(const char* format, ...)
{
    char buf[1024];
    va_list va;
    va_start(va, format);
    vsnprintf(buf, sizeof(buf) - 1, format, va);
    buf[sizeof(buf) - 1] = 0;
    va_end(va);
    return buf;
}

} // namespace Poseidon::Foundation
