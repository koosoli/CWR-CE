#pragma once

#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#include <Poseidon/Foundation/Common/Global.hpp>
#include <Poseidon/Foundation/Framework/LogFlags.hpp>

#if !defined(_WIN32) || defined NET_LOG

namespace Poseidon::Foundation
{
// General-purpose logger. Writes "netNNN.log" text files (NNN = 001, 002, …).
class NetLogger
{
  protected:
    FILE* f;

    unsigned counter; ///< for fflush.

    PoCriticalSection cs; ///< for exclusivity.

    unsigned64 startTime; ///< in getSystemTime() format (in microseconds).

    void open();

  public:
    NetLogger();

    ~NetLogger();

    void log(const char* format, va_list argptr);

    double getTime() const;
};

} // namespace Poseidon::Foundation

#endif // NET_LOG

