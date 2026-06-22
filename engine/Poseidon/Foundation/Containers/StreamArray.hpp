#pragma once

// Type must be able to:
// know size of instance: int ItemSize() const
// duplicate any instance: void Duplicate( Type &dst ) const

#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Containers/StaticArray.hpp>
#include <Poseidon/Foundation/Algorithms/Qsort.hpp>

namespace Poseidon::Foundation
{

#ifdef _DEBUG

class Offset
{
  private:
    int _offset;

  public:
    Offset() { _offset = 0; }
    explicit Offset(int offset) : _offset(offset) {}
    void Advance(int a) { _offset += a; }
    int GetOffset() const { return _offset; }

    bool operator<(Offset c) const { return _offset < c._offset; }
    bool operator<=(Offset c) const { return _offset <= c._offset; }
    bool operator==(Offset c) const { return _offset == c._offset; }
    bool operator>=(Offset c) const { return _offset >= c._offset; }
    bool operator>(Offset c) const { return _offset > c._offset; }
    bool operator!=(Offset c) const { return _offset != c._offset; }
};
#define O(i) ((i).GetOffset())
#define A(i, n) ((i).Advance(n))

inline int OffsetDiff(Offset a, Offset b)
{
    return a.GetOffset() - b.GetOffset();
}


// Allow fmt/spdlog to format Offset as its underlying integer (debug builds only)
inline int format_as(Offset o)
{
    return o.GetOffset();
}
#else
typedef int Offset; // better optimized, no safety
#define O(i) (i)
#define A(i, n) ((i) += (n))
#define OffsetDiff(a, b) (a) - (b)
#endif

struct StreamSortSegment
{
    Offset beg, end;

    void Clear() { beg = end = Offset(0); }
};


    // adapt a per-element ordering function into a per-segment one

    template <class Type, class Container, class Function>
    class CompareByOffset
{
  public:
    int operator()(const StreamSortSegment* s1, const StreamSortSegment* s2, Container* array) const
    {
        Function function;
        return function(&array->Get(s1->beg), &array->Get(s2->beg));
    }
};

template <class Type, class Container>
class StreamArray
{
    // Type must be simple - no constructor or destructor, trivially movable.
  private:
    Container _data;
    int _count{0};

    void DoConstruct(const StreamArray& src);

  public:
    StreamArray() = default;
    ~StreamArray() { Clear(); }

    void operator=(const StreamArray& src)
    {
        Clear();
        DoConstruct(src);
    }
    StreamArray(const StreamArray& src) { DoConstruct(src); }
    void Move(StreamArray& src);

    void Copy(const Type* src, int n);
    void Merge(const StreamArray& src);
    void Merge(const char* data, int size, int n);

    // read access functions
    Offset Begin() const { return Offset(0); }
    Offset End() const { return Offset(_data.Size()); }
    void Next(Offset& index) const { A(index, Get(index).ItemSize()); }
    const Type& Get(Offset pos) const { return reinterpret_cast<const Type&>(_data[O(pos)]); }
    Type& Set(Offset pos) { return reinterpret_cast<Type&>(_data[O(pos)]); }

    const Type& operator[](Offset pos) const { return Get(pos); }
    Type& operator[](Offset pos) { return Set(pos); }

    Offset Find(int index) const;
    int Size() const { return _count; }

    int CalculateCount() const;

    Container& GetData() { return _data; }
    const Container& GetData() const { return _data; }

    const char* RawData() const { return _data.Data(); }
    int RawSize() const { return _data.Size(); }
    void RawRealloc(int n) { _data.Realloc(n); }

    // write access functions
    Offset Add(const Type& src) // add single
    {
        int pos = _data.Size();
        _data.Resize(pos + src.ItemSize());
        Type& dst = reinterpret_cast<Type&>(_data[pos]);
        src.Duplicate(dst);
        _count++;
        return Offset(pos);
    }
    void Clear() { _data.Clear(), _count = 0; }
    void Delete(Offset pos)
    {
        int size = Get(pos).ItemSize();
        // note: Container should be char storage
        PoseidonAssert(sizeof(typename Container::DataType) == 1);
        memmove(_data.Data() + O(pos), _data.Data() + (O(pos) + size), _data.Size() - (O(pos) + size));
        _data.Resize(_data.Size() - size);
        _count--;
    }

    void Compact() { _data.Realloc(_data.Size()); }
    void ReserveRaw(int size) { _data.Reserve(size, size); }
    void Reserve(int n)
    {
        int size = Type::TypicalItemSize() * n;
        _data.Reserve(size, size);
    }
    void ReserveMore(int n)
    {
        int size = O(End()) + Type::TypicalItemSize() * n;
        _data.Reserve(size);
    }
    // use Realloc with caution
    void Realloc(int n)
    {
        int size = Type::TypicalItemSize() * n;
        int minSize = _data.Size();
        if (size < minSize)
        {
            size = minSize;
        }
        _data.Realloc(size);
    }

    // sort - slow, O(n^2)
    void Sort(int (*compare)(const Type* a, const Type* b));
    // Scan for runs with identical key, then qsort the runs.
    template <class Compare>
    void QSortSeq(Compare compare)
    {
        AUTO_STATIC_ARRAY(StreamSortSegment, index, 256);
        StreamSortSegment seg;
        seg.Clear();
        for (Offset i = Begin(); i < End(); Next(i))
        {
            if (seg.end <= seg.beg)
            {
                // empty segment - start a new one
                seg.beg = i;
                seg.end = i;
                Next(seg.end);
            }
            else
            {
                // check if we are in the same segment
                const Type& segStart = Get(seg.beg);
                const Type& face = Get(i);
                if (compare(&face, &segStart))
                {
                    // different texture
                    index.Add(seg);
                    seg.Clear();
                    // start a new segment
                    seg.beg = i;
                }
                // extend current segment to contain this face
                seg.end = i;
                Next(seg.end);
            }
        }
        if (seg.end > seg.beg)
        {
            index.Add(seg);
        }
        if (index.Size() > 1)
        {
            // if index is singular, there is no need to sort and copy the array
            // now sort face index using QSort
            CompareByOffset<Type, StreamArray, Compare> contextCompare;
            QSortWithContext(index.Data(), index.Size(), this, contextCompare);
            // and finally: stream face data based on face index to the destination

            StreamArray<Type, Container> copy = *this;
            Clear();
            RawRealloc(copy.RawSize());

            for (int i = 0; i < index.Size(); i++)
            {
                const StreamSortSegment& s = index[i];
                // copy s.beg .. s.end into destination
                for (Offset o = s.beg; o < s.end; copy.Next(o))
                {
                    Add(copy[o]);
                }
            }
        }
    }
};

template <class Type, class Container>
void StreamArray<Type, Container>::DoConstruct(const StreamArray& src)
{
    _data = src._data;
    _count = src._count;
}

template <class Type, class Container>
void StreamArray<Type, Container>::Copy(const Type* src, int n)
{
    Reserve(n);
    _data.Resize(0);
    for (int i = 0; i < n; i++)
        Add(src[i]);
}

template <class Type, class Container>
void StreamArray<Type, Container>::Move(StreamArray& src)
{
    // src will be clear
    _count = src._count;
    src._count = 0;
    // transfer actual data
    _data.Move(src._data);
}

template <class Type, class Container>
void StreamArray<Type, Container>::Merge(const StreamArray& src)
{
    PoseidonAssert(sizeof(typename Container::DataType) == 1);
    int offset = _data.Size();
    _data.Resize(offset + src._data.Size());
    memcpy(_data.Data() + offset, src._data.Data(), src._data.Size());
    _count += src._count;
}

template <class Type, class Container>
void StreamArray<Type, Container>::Merge(const char* data, int size, int n)
{
    PoseidonAssert(sizeof(typename Container::DataType) == 1);
    int offset = _data.Size();
    _data.Resize(offset + size);
    memcpy(_data.Data() + offset, data, size);
    _count += n;
}

template <class Type, class Container>
Offset StreamArray<Type, Container>::Find(int index) const
{
    int i = 0;
    for (Offset f = Begin(), e = End(); f < e; Next(f), i++)
    {
        if (i == index)
        {
            return f;
        }
    }
    Fail("No entry");
    return End();
}

template <class Type, class Container>
int StreamArray<Type, Container>::CalculateCount() const
{
    int i = 0;
    for (Offset f = Begin(), e = End(); f < e; Next(f), i++)
    {
    }
    return i;
}

template <class Type, class Container>
void StreamArray<Type, Container>::Sort(int (*compare)(const Type* a, const Type* b))
{
    StreamArray copy = *this;
    Clear();
    RawRealloc(copy.RawSize());
    for (;;)
    {
        // find lowest of copy, move it into result
        Offset lowest = copy.Begin();
        Offset current = lowest, end = copy.End();
        // check if there is something left
        if (!(current < end))
        {
            return;
        }
        for (copy.Next(current); current < end; copy.Next(current))
        {
            if (compare(&copy.Get(current), &copy.Get(lowest)) < 0)
            {
                lowest = current;
            }
        }
        const Type& poly = copy.Get(lowest);
        Add(poly);
        copy.Delete(lowest);
    }
}

#undef O
#undef A

} // namespace Poseidon::Foundation

using ::Poseidon::Foundation::StreamArray;
using ::Poseidon::Foundation::Offset;
using ::Poseidon::Foundation::StreamSortSegment;
