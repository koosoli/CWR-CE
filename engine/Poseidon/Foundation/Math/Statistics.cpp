#include <Poseidon/Core/Application.hpp>

#include <Poseidon/Foundation/Math/Statistics.hpp>
#include <Poseidon/Foundation/Algorithms/Qsort.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <Poseidon/Foundation/Containers/SmallArray.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>

// StatisticsAvg allows tracking basic statistics of single value

namespace Poseidon::Foundation
{
void StatisticsAvg::Clear()
{
    _min = INT_MAX, _max = INT_MIN, _sum = 0, _n = 0;
}
void StatisticsAvg::Count(int value, int n)
{
    if (_min > value)
    {
        _min = value;
    }
    if (_max < value)
    {
        _max = value;
    }
    _sum += value;
    _n += n;
    if (_n >= _after)
    {
        Report(), Clear();
    }
}

void StatisticsAvg::Report()
{
    if (_n > 0)
    {
        LOG_DEBUG(Core, "{} {}..{}, avg {:.1f}", _name, _min, _max, (float)_sum / _n);
    }
}

// StatisticsByName allows tracking histogram (by name)

static int CompareItem(const StatisticsByName::Item* t1, const StatisticsByName::Item* t2)
{
    return t2->count - t1->count;
}

static int CompareName(const StatisticsByName::Item* t1, const StatisticsByName::Item* t2)
{
    return strcmp(t1->name, t2->name);
}

int StatisticsByName::Find(const char* name)
{
    for (int i = 0; i < _data.Size(); i++)
    {
        if (!strcmp(name, _data[i].name))
        {
            return i;
        }
    }
    return -1;
}

void StatisticsByName::Sample(int factor)
{
    _factor += factor;
}

void StatisticsByName::Count(const char* name, int count)
{
    char shortName[Item::LenName];
    strncpy(shortName, name, sizeof(shortName));
    shortName[sizeof(shortName) - 1] = 0;
    int index = Find(shortName);
    if (index < 0)
    {
        index = _data.Add();
        if (index < 0)
        {
            Fail("StatisticsByName buffer too small");
            return;
        }
        Item& item = _data[index];
        strcpy(item.name, shortName);
        item.count = 0;
    }
    _data[index].count += count;
}

StatisticsByName::StatisticsByName()
{
    _factor = 0;
}
StatisticsByName::~StatisticsByName() = default;

void StatisticsByName::Clear()
{
    _data.Clear();
}

void StatisticsByName::Report(QOStream& f, bool nonZeroOnly, bool sortByName)
{
    if (_data.Size() <= 0)
    {
        return;
    }
    if (_factor < 1)
    {
        _factor = 1;
    }
    Poseidon::Foundation::QSort(&_data[0], _data.Size(), sortByName ? CompareName : CompareItem);
    for (int i = 0; i < _data.Size(); i++)
    {
        if (nonZeroOnly && _data[i].count == 0)
        {
            continue;
        }
        char buf[1024];
        snprintf(buf, sizeof(buf), "%32s: %6d\r\n", _data[i].name, _data[i].count / _factor);
        f.write(buf, strlen(buf));
    }
}

void StatisticsByName::Report(bool nonZeroOnly, bool sortByName)
{
    if (_data.Size() <= 0)
    {
        return;
    }
    if (_factor < 1)
    {
        _factor = 1;
    }
    Poseidon::Foundation::QSort(&_data[0], _data.Size(), sortByName ? CompareName : CompareItem);
    for (int i = 0; i < _data.Size(); i++)
    {
        if (nonZeroOnly && _data[i].count == 0)
        {
            continue;
        }
        LOG_DEBUG(Core, "{:>32}: {:6}", _data[i].name, _data[i].count / _factor);
    }
}

class AllocatorDefault
{
  public:
    char* Alloc(int size, int& retSize)
    {
        retSize = size;
        return new char[size];
    }
    void Free(char* mem) { delete[] mem; }
};

class AllocatorStatic : private AllocatorDefault
{
    typedef AllocatorDefault base;

    char* _buf;
    int _size;
    bool _free;

  public:
    AllocatorStatic(char* buf, int size);
    char* Alloc(int size, int& retSize);
    void Free(char* mem);
};

AllocatorStatic::AllocatorStatic(char* buf, int size)
{
    _buf = buf;
    _size = size;
    _free = true;
}

char* AllocatorStatic::Alloc(int size, int& retSize)
{
    if (_free && size <= _size)
    {
        _free = false;
        retSize = _size;
        return _buf;
    }
    return base::Alloc(size, retSize);
}

void AllocatorStatic::Free(char* mem)
{
    if (mem == _buf)
    {
        _free = true;
        return;
    }
    base::Free(mem);
}

} // namespace Poseidon::Foundation
