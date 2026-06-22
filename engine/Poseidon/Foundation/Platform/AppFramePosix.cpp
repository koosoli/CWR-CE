#include <Poseidon/Core/Application.hpp>
#include <Poseidon/Foundation/Framework/AppFrame.hpp>

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/Strings/Bstring.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <time.h>
#include <Poseidon/Dev/Debug/DebugTrap.hpp>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Logging/Logging.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon::Foundation
{
using namespace Poseidon::Dev;

class LinuxFrameFunctions : public AppFrameFunctions
{
  protected:
    unsigned64 startTime;

  public:
    LinuxFrameFunctions() { startTime = getSystemTime(); }

    void ErrorMessage(const char* format, va_list argptr) override;
    void ErrorMessage(ErrorMessageLevel level, const char* format, va_list argptr) override;
    void WarningMessage(const char* format, va_list argptr) override;

    DWORD TickCount() override
    {
        return ((DWORD)((getSystemTime() - startTime + 500) / 1000)); // 1 ms resolution
    }
};

static LinuxFrameFunctions GAppFrameFunctions INIT_PRIORITY_URGENT;
AppFrameFunctions* CurrentAppFrameFunctions = &GAppFrameFunctions;

extern void DDTerm();
extern void WarningMessageLevel(ErrorMessageLevel level, const char* format, va_list argptr);

void LinuxFrameFunctions::ErrorMessage(const char* format, va_list argptr)
{
    static int avoidRecursion = 0;
    if (avoidRecursion++ != 0)
    {
        return;
    }
    BString<256> buf;
    vsprintf(buf, format, argptr);
    // Critical engine error. Fatal under --strict; logged-and-continue under --no-strict
    // (the build players run). The ERROR log also trips the strict latch under --strict.
    LOG_ERROR(Core, "Critical error: {}", (const char*)buf);

    if (!LoggingSystem::IsStrictMode())
    {
        avoidRecursion = 0; // a later, distinct critical error must still report
        return;
    }

    GDebugger.NextAliveExpected(15 * 60 * 1000);
    DDTerm();
    fputs(buf, stderr);
    fputc('\n', stderr);
    exit(1);
}

void LinuxFrameFunctions::ErrorMessage(ErrorMessageLevel level, const char* format, va_list argptr)
{
    if (level <= EMError)
    {
        WarningMessageLevel(level, format, argptr);
    }
    else
    {
        ErrorMessage(format, argptr);
    }
}

void LinuxFrameFunctions::WarningMessage(const char* format, va_list argptr)
{
    WarningMessageLevel(EMWarning, format, argptr);
}

} // namespace Poseidon::Foundation
