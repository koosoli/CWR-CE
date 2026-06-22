#pragma once

#include <Poseidon/Foundation/Containers/SmallArray.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

namespace Poseidon::Foundation
{
class StatisticsByName
{
  public:
    struct Item
    {
        enum
        {
            LenName = 128 - 4
        };
        char name[LenName];
        int count;
    };

  private:
    VerySmallArray<Item, 4 * 1024 * sizeof(Item)> _data;
    int _factor; // divide data by factor - enables avg calculation

    int Find(const char* name);

  public:
    StatisticsByName();
    ~StatisticsByName();

    void Clear();
    void Count(const char* name, int count = 1);
    void Report(bool nonZeroOnly = false, bool sortByName = false);
    void Report(QOStream& f, bool nonZeroOnly = false, bool sortByName = false);
    void Sample(int factor = 1);
    int NSamples() const { return _factor; }
};

class StatisticsAvg
{
    const char* _name;
    int _min, _max, _sum, _n;
    int _after;

  public:
    StatisticsAvg(const char* name, int after = 8 * 1024) : _name(name), _after(after) {}
    void Clear();
    void Count(int value, int n = 1);
    void Report();
};

class StatEventRatio
{
    const char* _name;
    int _event, _total, _maxCount;

  public:
    StatEventRatio(const char* event, int maxCount = 1000)
    {
        _name = event, _event = _total = 0;
        _maxCount = maxCount;
    }
    void Count(bool happened)
    {
        _event += happened;
        _total += 1;
        if (_total > _maxCount)
        {
            LOG_DEBUG(Core, "{} ratio: {:.3f}", _name, _event * 1.0f / _total);
            _total = 0;
            _event = 0;
        }
    }
};

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::StatEventRatio;
using ::Poseidon::Foundation::StatisticsAvg;
using ::Poseidon::Foundation::StatisticsByName;
