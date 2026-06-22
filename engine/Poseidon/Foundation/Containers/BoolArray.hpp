#pragma once


#include <Poseidon/Foundation/platform.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
namespace Poseidon::Foundation
{
template <class Enum, class Type>
class SizedEnum
{
    Type _data;

  public:
#ifdef __GNUC__
    operator Enum() const { return (Enum)(int)_data; }
    operator Type() const { return _data; }
#else
    operator Enum() const { return (Enum)_data; }
#endif
    SizedEnum(Enum val) : _data(val) {}
#ifdef __GNUC__
    SizedEnum(Type val) : _data(val) {}
#endif
    SizedEnum() = default;
} PACKED;

class PackedBoolArray
{
  private:
    int _data;

  public:
    enum
    {
        NBools = sizeof(int) * 8
    };
    PackedBoolArray() { Init(); }
    void Init() { _data = 0; }
    bool Get(int pos) const
    {
        PoseidonAssert(pos >= 0 && pos < NBools);
        return (_data & (1 << pos)) != 0;
    }
    void Set(int pos, bool value)
    {
        PoseidonAssert(pos >= 0 && pos < NBools);
        if (value)
        {
            _data |= (1 << pos);
        }
        else
        {
            _data &= (~(1 << pos));
        }
    }
    bool operator[](int i) const { return Get(i); }

    void Toggle(int pos)
    {
        PoseidonAssert(pos >= 0 && pos < NBools);
        _data ^= (1 << pos);
    }
    bool IsEmpty() const { return _data == 0; }
    int GetCount() const
    {
        int c = 0;
        for (int i = 0; i < NBools; i++)
        {
            if (Get(i))
            {
                c++;
            }
        }
        return c;
    }

    bool operator==(const PackedBoolArray dst) const { return _data == dst._data; }
    bool IsPartOf(const PackedBoolArray dst) const { return (_data & dst._data) == _data; }
    bool Contain(const PackedBoolArray dst) const { return (_data & dst._data) == dst._data; }
};


class PackedBoolAutoArray
{
    enum
    {
        PerItem = PackedBoolArray::NBools
    };
    AutoArray<PackedBoolArray> _data;

  public:
    bool Get(int pos) const
    {
        int index = pos / PerItem;
        if (index >= _data.Size())
        {
            return false;
        }
        return _data.Get(index).Get(pos - index * PerItem);
    }
    void Set(int pos, bool value)
    {
        int index = pos / PerItem;
        _data.Access(index);
        _data.Set(index).Set(pos - index * PerItem, value);
    }
    bool operator[](int i) const { return Get(i); }
    void Clear() { _data.Clear(); }
    bool IsEmpty() const
    {
        for (int i = 0; i < _data.Size(); i++)
        {
            if (!_data[i].IsEmpty())
            {
                return false;
            }
        }
        return true;
    }
};

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::SizedEnum;
using ::Poseidon::Foundation::PackedBoolArray;
