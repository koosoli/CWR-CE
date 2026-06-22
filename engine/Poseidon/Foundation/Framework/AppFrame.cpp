#include <Poseidon/Foundation/Framework/AppFrame.hpp>

#include <stdio.h>
#include <string.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>


namespace Poseidon::Foundation
{
bool gSoftAssert = false;

void ErrorMessage(ErrorMessageLevel level, const char* format, ...)
{
    va_list a;
    va_start(a, format);
    CurrentAppFrameFunctions->ErrorMessage(level, format, a);
    va_end(a);
}

void ErrorMessage(const char* format, ...)
{
    va_list a;
    va_start(a, format);
    CurrentAppFrameFunctions->ErrorMessage(format, a);
    va_end(a);
}

void WarningMessage(const char* format, ...)
{
    va_list a;
    va_start(a, format);
    CurrentAppFrameFunctions->WarningMessage(format, a);
    va_end(a);
}

void GlobalShowMessage(int timeMs, const char* msg, ...)
{
    va_list a;
    va_start(a, msg);
    CurrentAppFrameFunctions->ShowMessage(timeMs, msg, a);
    va_end(a);
}

DWORD GlobalTickCount()
{
    return CurrentAppFrameFunctions->TickCount();
}

} // namespace Poseidon::Foundation
