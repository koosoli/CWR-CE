#pragma once

#include <Poseidon/Network/NetTransportStatisticsFormatting.hpp>

#include <cstdio>
#include <utility>
#include <Poseidon/Network/NetTransportMetrics.hpp>

namespace Poseidon
{

template <class ReadMetricsFn, class MissingTextFn, class LogFn>
RString QueryNetTransportStatistics(ReadMetricsFn&& readMetrics, MissingTextFn&& missingText, bool shouldLog,
                                    unsigned64 now, bool advanceNextStatLog, unsigned64& nextStatLog,
                                    int& getStatisticsCount, LogFn&& logFn)
{
    NetworkTransportMetrics metrics{};
    if (!readMetrics(metrics))
    {
        return missingText();
    }

    return FormatNetTransportStatistics(metrics, shouldLog, now, advanceNextStatLog, nextStatLog, getStatisticsCount,
                                        std::forward<LogFn>(logFn));
}

template <class ReadMetricsFn, class MissingTextFn>
RString QueryNetTransportStatisticsWithoutLogging(ReadMetricsFn&& readMetrics, MissingTextFn&& missingText,
                                                  unsigned64 now)
{
    unsigned64 ignoredNextStatLog = 0;
    int ignoredStatisticsCount = 0;
    return QueryNetTransportStatistics(std::forward<ReadMetricsFn>(readMetrics),
                                       std::forward<MissingTextFn>(missingText), false, now, false, ignoredNextStatLog,
                                       ignoredStatisticsCount, [](const RString&, int) {});
}

template <class ReadMetricsFn, class NowFn>
RString GetNetTransportClientStatistics(ReadMetricsFn&& readMetrics, NowFn&& getNow)
{
    return QueryNetTransportStatisticsWithoutLogging(
        std::forward<ReadMetricsFn>(readMetrics),
        []() { return RString("No NetChannel is connected to this NetClient"); }, std::forward<NowFn>(getNow)());
}

template <class ReadMetricsFn, class NowFn>
RString GetNetTransportServerStatistics(int player, ReadMetricsFn&& readMetrics, NowFn&& getNow)
{
    return QueryNetTransportStatisticsWithoutLogging(
        std::forward<ReadMetricsFn>(readMetrics),
        [player]()
        {
            char text[64];
            std::snprintf(text, sizeof(text), "Unknown player ID = %d", player);
            return RString(text);
        },
        std::forward<NowFn>(getNow)());
}

} // namespace Poseidon
