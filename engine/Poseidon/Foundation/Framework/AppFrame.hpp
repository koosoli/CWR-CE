#pragma once

#include <stdarg.h>
#include <Poseidon/Foundation/Types/Memtype.h>

namespace Poseidon::Foundation
{
enum ErrorMessageLevel
{
    EMNote,      // marginal impact
    EMWarning,   // limited functionality
    EMError,     // cannot perform task
    EMCritical,  // cannot continue
    EMDisableAll // disable all errors
};

void ErrorMessage(ErrorMessageLevel level, const char* format, ...);
void ErrorMessage(const char* format, ...);
void WarningMessage(const char* format, ...);

void GlobalShowMessage(int timeMs, const char* msg, ...);
DWORD GlobalTickCount();

class AppFrameFunctions
{
  public:
    AppFrameFunctions() = default;
    virtual ~AppFrameFunctions() = default;

    virtual void ErrorMessage(ErrorMessageLevel level, const char* format, va_list argptr)
    {
        (void)level;
        (void)format;
        (void)argptr;
    }
    virtual void ErrorMessage(const char* format, va_list argptr)
    {
        (void)format;
        (void)argptr;
    }
    virtual void WarningMessage(const char* format, va_list argptr)
    {
        (void)format;
        (void)argptr;
    }
    virtual void ShowMessage(int timeMs, const char* msg, va_list argptr)
    {
        (void)timeMs;
        (void)msg;
        (void)argptr;
    }
    virtual DWORD TickCount() { return 0; }
};

extern AppFrameFunctions* CurrentAppFrameFunctions;
#define CurrentAppFrameFunctions() CurrentAppFrameFunctions

} // namespace Poseidon::Foundation

using Poseidon::Foundation::ErrorMessageLevel;
using Poseidon::Foundation::EMNote;
using Poseidon::Foundation::EMWarning;
using Poseidon::Foundation::EMError;
using Poseidon::Foundation::EMCritical;
using Poseidon::Foundation::EMDisableAll;
using Poseidon::Foundation::ErrorMessage;
using Poseidon::Foundation::WarningMessage;
using Poseidon::Foundation::GlobalShowMessage;
using Poseidon::Foundation::GlobalTickCount;
using Poseidon::Foundation::AppFrameFunctions;
using Poseidon::Foundation::CurrentAppFrameFunctions;

