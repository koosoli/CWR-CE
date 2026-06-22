#pragma once

#ifndef _STATISTICS_H // Guard against Poseidon's statistics.hpp

class StatisticsByName
{
  public:
    StatisticsByName();
    ~StatisticsByName();

    void Count(const char* name, int count = 1);
};

#endif // _STATISTICS_H
