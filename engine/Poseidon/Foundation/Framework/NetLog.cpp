#include <time.h>
#include <Poseidon/Foundation/Framework/NetLog.hpp>
#include <Poseidon/Foundation/Framework/PoTime.hpp>
#include <string.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Framework/LogFlags.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/platform.hpp>
#include <cstdarg>

bool netLogValid = true;

namespace Poseidon::Foundation
{

#if !defined(_WIN32) || defined NET_LOG

NetLogger::NetLogger() : LockInit(cs, "NetLogger", true)
{
    cs.enter();
    f = nullptr;
    startSystemTime();
    startTime = getSystemTime();
    netLogValid = true;
    f = fopen("nolog.txt", "rt");
    if (f)
    {
        fclose(f);
        f = nullptr;
        netLogValid = false;
    }
    cs.leave();
}

void NetLogger::open()
{
    if (f)
        return;
    cs.enter();
    int i = 1;
    char buf[32];
#ifdef NET_LOG_BRIEF
    f = fopen("net.log", "a+t");
#else
    do
    {
        snprintf(buf, sizeof(buf), "net%03d.log", i++);
        f = fopen(buf, "rt");
        if (!f)
            break;
        fclose(f);
    } while (true);
    f = fopen(buf, "wt");
#endif
    counter = 0;
    // initial message:
    time_t timet;
    time(&timet);
    timet -= (time_t)((getSystemTime() - startTime) / 1000000);
    snprintf(buf, sizeof(buf), "%s", (const char*)ctime(&timet));
    buf[strlen(buf) - 1] = (char)0; // remove trailing '\n'
#ifdef NET_LOG_BRIEF
    fprintf(f, "%9.3f: NetLogger: start - %s\n", 0.0, buf);
    fprintf(f, "%9.3f: Clk(%u)\n", 0.0, getClockFrequency());
#else
    fprintf(f, "%10.5f: NetLogger: start - %s\n", 0.0, buf);
    fprintf(f, "%10.5f: Clock frequency = %u Hz\n", 0.0, getClockFrequency());
#endif
    cs.leave();
}

NetLogger::~NetLogger()
{
    if (!netLogValid || !f)
        return;
    char buf[32];
    time_t timet;
    time(&timet);
    snprintf(buf, sizeof(buf), "%s", (const char*)ctime(&timet));
    buf[strlen(buf) - 1] = (char)0; // remove trailing '\n'
    netLog("NetLogger: stop - %s", buf);
    netLogValid = false;
    cs.enter();
    if (f)
    {
        fclose(f);
        f = nullptr;
    }
    cs.leave();
}

double NetLogger::getTime() const
{
    return (1.e-6 * (getSystemTime() - startTime));
}

void NetLogger::log(const char* format, va_list argptr)
{
    cs.enter();
    if (netLogValid)
    {
        if (!f)
            open(); // deferred file open
        PoseidonAssert(f);
#ifdef NET_LOG_BRIEF
        fprintf(f, "%9.3f: ", getTime());
#else
        fprintf(f, "%10.5f: ", getTime());
#endif
        vfprintf(f, format, argptr);
        fputc('\n', f);
#ifndef IMMEDIATE_NET_LOG
#ifdef NET_LOG_BRIEF
        if (!(++counter & 0x007))
#else
        if (!(++counter & 0x1ff))
#endif
#endif
            fflush(f);
    }
    cs.leave();
}

#ifdef _WIN32

#ifdef EXTERN_NET_LOG
extern
#endif
    NetLogger netLogger;

#else

NetLogger netLogger INIT_PRIORITY_URGENT;

#endif

void netLog(const char* format, ...)
{
    if (!netLogValid)
        return;
    va_list arglist;
    va_start(arglist, format);
    netLogger.log(format, arglist);
    va_end(arglist);
}

double getLogTime()
{
    if (!netLogValid)
        return 0.0;
    return netLogger.getTime();
}

#else

double getLogTime()
{
    return 0.0;
}

#endif

} // namespace Poseidon::Foundation
