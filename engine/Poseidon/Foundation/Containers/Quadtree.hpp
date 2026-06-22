#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>

namespace Poseidon::Foundation
{
template <class Type, class Traits = FindArrayKeyTraits<Type>, class Allocator = Poseidon::Foundation::MemAllocD>
class QuadTree
{
  protected:
    FindArrayKey<Type, Traits, Allocator> _values;
    AutoArray<int, Allocator> _index;
    AutoArray<int, Allocator> _free;

    Type _defaultValue;
    int _maxLevel;
    int _rootIndex;

  public:
    // defaultValue is returned for out-of-bounds coordinates.
    QuadTree(const Type& defaultValue) : _defaultValue(defaultValue) { Clear(); }
    void Clear();

    const Type& Get(int x, int y);
    void Set(int x, int y, const Type& value);

  protected:
    int AddNode();
    int IsEqual(const Type& a, const Type& b) { return Traits::IsEqual(Traits::GetKey(a), Traits::GetKey(b)); }
};

template <class Type, class Traits, class Allocator>
void QuadTree<Type, Traits, Allocator>::Clear()
{
    _values.Clear();
    _index.Clear();
    _free.Clear();

    // _values[0] is always default value
    _values.Add(_defaultValue);

    _maxLevel = 0;
    _rootIndex = -1;
}

template <class Type, class Traits, class Allocator>
const Type& QuadTree<Type, Traits, Allocator>::Get(int x, int y)
{
    int size = 1 << _maxLevel;

    // check bounds
    if ((x | y) & ~(size - 1))
    {
        return _defaultValue; // out of bounds
    }

    int startX = 0, startY = 0;

    int index = _rootIndex;
    while (index >= 0)
    {
        size >>= 1;
        if (x < startX + size)
        {
            if (y < startY + size)
            {
                index = _index[index + 0];
            }
            else
            {
                index = _index[index + 1];
                startY += size;
            }
        }
        else
        {
            if (y < startY + size)
            {
                index = _index[index + 2];
            }
            else
            {
                index = _index[index + 3];
                startY += size;
            }
            startX += size;
        }
    }
    PoseidonAssert(index < 0 && -index <= _values.Size());
    return _values[-index - 1];
}

template <class Type, class Traits, class Allocator>
void QuadTree<Type, Traits, Allocator>::Set(int x, int y, const Type& value)
{
    int size = 1 << _maxLevel;

    // check bounds
    if ((x | y) & ~(size - 1))
    {
        if (IsEqual(value, _defaultValue))
        {
            return;
        }
        do
        {
            // out of bounds - extend
            int n = AddNode();
            _index[n + 0] = _rootIndex;
            _index[n + 1] = -1; // default value
            _index[n + 2] = -1; // default value
            _index[n + 3] = -1; // default value

            _rootIndex = n;
            _maxLevel++;
            size <<= 1;
        } while ((x | y) & ~(size - 1));
    }

    // find current leaf
    int startX = 0, startY = 0;

    int pathIndex = 0;
    int path[8 * sizeof(int)];

    int* index = &_rootIndex;
    while (*index >= 0)
    {
        path[pathIndex++] = *index;

        size >>= 1;
        if (x < startX + size)
        {
            if (y < startY + size)
            {
                index = &_index[*index + 0];
            }
            else
            {
                index = &_index[*index + 1];
                startY += size;
            }
        }
        else
        {
            if (y < startY + size)
            {
                index = &_index[*index + 2];
            }
            else
            {
                index = &_index[*index + 3];
                startY += size;
            }
            startX += size;
        }
    }

    // check if change must be done
    int currentIndex = *index;
    int indexValue = -currentIndex - 1;
    PoseidonAssert(indexValue >= 0 && indexValue < _values.Size());
    if (IsEqual(_values[indexValue], value))
    {
        return;
    }

    // aggregation can be done
    bool aggregate = size == 1;

    // split current leaf node
    while (size > 1)
    {
        // add new node to hierarchy
        int n = AddNode();
        *index = n;

        size >>= 1;
        if (x < startX + size)
        {
            if (y < startY + size)
            {
                _index[*index + 1] = currentIndex;
                _index[*index + 2] = currentIndex;
                _index[*index + 3] = currentIndex;
                index = &_index[*index + 0];
            }
            else
            {
                _index[*index + 0] = currentIndex;
                _index[*index + 2] = currentIndex;
                _index[*index + 3] = currentIndex;
                index = &_index[*index + 1];
                startY += size;
            }
        }
        else
        {
            if (y < startY + size)
            {
                _index[*index + 0] = currentIndex;
                _index[*index + 1] = currentIndex;
                _index[*index + 3] = currentIndex;
                index = &_index[*index + 2];
            }
            else
            {
                _index[*index + 0] = currentIndex;
                _index[*index + 1] = currentIndex;
                _index[*index + 2] = currentIndex;
                index = &_index[*index + 3];
                startY += size;
            }
            startX += size;
        }
    }

    // set new value
    indexValue = _values.Find(value);
    if (indexValue < 0)
    {
        indexValue = _values.Add(value);
    }
    currentIndex = -indexValue - 1;
    *index = currentIndex;

    // aggregation
    if (aggregate)
    {
        for (int i = pathIndex - 1; i >= 0; i--)
        {
            int index = path[i];
            if (_index[index + 0] != currentIndex)
            {
                break;
            }
            if (_index[index + 1] != currentIndex)
            {
                break;
            }
            if (_index[index + 2] != currentIndex)
            {
                break;
            }
            if (_index[index + 3] != currentIndex)
            {
                break;
            }

            // aggregation is possible
            _free.Add(index);
            if (i == 0)
            {
                PoseidonAssert(_rootIndex == index);
                _rootIndex = currentIndex;
            }
            else
            {
                int levelUp = path[i - 1];
                if (_index[levelUp + 0] == index)
                {
                    _index[levelUp + 0] = currentIndex;
                }
                else if (_index[levelUp + 1] == index)
                {
                    _index[levelUp + 1] = currentIndex;
                }
                else if (_index[levelUp + 2] == index)
                {
                    _index[levelUp + 2] = currentIndex;
                }
                else if (_index[levelUp + 3] == index)
                {
                    _index[levelUp + 3] = currentIndex;
                }
            }
        }
    }
    int i = _rootIndex;
    while (i >= 0 && _index[i + 1] == -1 && _index[i + 2] == -1 && _index[i + 3] == -1)
    {
        _free.Add(i);
        _rootIndex = _index[i + 0];
        _maxLevel--;
        i = _rootIndex;
    }
}

template <class Type, class Traits, class Allocator>
int QuadTree<Type, Traits, Allocator>::AddNode()
{
    int n = _free.Size();
    if (n > 0)
    {
        int i = _free[n - 1];
        _free.Delete(n - 1);
        return i;
    }

    int i = _index.Size();
    _index.Access(i + 3);
    return i;
}

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::QuadTree;

