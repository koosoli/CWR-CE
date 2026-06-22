#pragma once

// Allocator policy classes for the container templates (AutoArray, HeapArray,
// FindArray, SortedArray):
// - MemAllocD:   plain heap (new[]/delete[]) — the common case
// - MemAllocS:   static pre-allocated buffer, heap fallback when it overflows
// - MemAllocSS:  shares one MemAllocS across containers; heap when none set
// - MemAllocSA:  caller-supplied (stack) buffer, heap fallback
// - MStorage<T>: typed sizing helper over MemAllocS

#include <Poseidon/Foundation/Framework/DebugLog.hpp>


namespace Poseidon::Foundation
{
class MemAllocD
{
  public:
    static void* Alloc(int& size) { return new char[size]; }
    static void Free(void* mem, int size)
    {
        (void)size;
        delete[] (char*)mem;
    }
    static void Free(void* mem) { delete[] (char*)mem; }
    static void Unlink(void* mem) { (void)mem; }
};

class MemAllocS
{
    void* _mem{nullptr};
    int _size{0};
    MemAllocD _fallBack;

  public:
    void* Alloc(int& size)
    {
        if (size <= _size)
        {
            size = _size;
            _size = -_size; // mark as allocated
            return _mem;
        }
        else
        {
            return _fallBack.Alloc(size);
        }
    }
    void Free(void* mem, int size)
    {
        if (mem == _mem)
        {
            PoseidonAssert(_size < 0);
            _mem = mem;
            _size = -_size;
        }
        else
        {
            _fallBack.Free(mem, size);
        }
    }
    MemAllocS() = default;
    ~MemAllocS() { Clear(); }

    MemAllocS* InitStorage(int size)
    {
        if (!_mem)
        {
            _mem = _fallBack.Alloc(size);
            _size = size;
        }
        return this;
    }

    void* GetMemory() const { return _mem; } // for diagnostic purposes only

    MemAllocS* InitStorage(int size, const char* name)
    {
        (void)name;
        if (!_mem)
        {
            _mem = _fallBack.Alloc(size);
            _size = size;
        }
        return this;
    }

    void Clear()
    {
        if (_mem)
        {
            DoAssert(_size >= 0);
            _fallBack.Free(_mem); // _size does not matter for MemAllocD
            _mem = nullptr;
            _size = 0;
        }
    }

    void Unlink(void* mem)
    {
        if (_mem == mem)
        {
            // memory is now dynamic - leave it to _fallback
            _mem = nullptr;
            // allocate new static memory
            InitStorage(_size < 0 ? -_size : _size, "Unlink");
        }
    }
};

class MemAllocSS // static shared
{
    MemAllocS* _alloc;

  public:
    void Init(MemAllocS* alloc) { _alloc = alloc; }
    MemAllocSS() : _alloc(nullptr) {}
    MemAllocSS(MemAllocS* alloc) : _alloc(alloc) {}

    void* Alloc(int& size)
    {
        if (_alloc)
        {
            return _alloc->Alloc(size);
        }
        else
        {
            return MemAllocD::Alloc(size);
        }
    }
    void Free(void* mem, int size)
    {
        if (_alloc)
        {
            _alloc->Free(mem, size);
        }
        else
        {
            MemAllocD::Free(mem);
        }
    }

    void Unlink(void* mem)
    {
        if (_alloc)
        {
            _alloc->Unlink(mem);
            _alloc = nullptr;
        }
    }
};

class MemAllocSA
{
    // memory allocated on stack

    void* _mem{nullptr};
    int _size{0};
    MemAllocD _fallBack;

  public:
    void* Alloc(int& size)
    {
        if (size <= _size)
        {
            size = _size;
            _size = -_size; // mark as allocated
            return _mem;
        }
        else
        {
            return _fallBack.Alloc(size);
        }
    }
    void Free(void* mem, int size)
    {
        if (mem == _mem)
        {
            // make size negative - this will disable any other requests
            PoseidonAssert(_size < 0);
            _mem = mem;
            _size = -_size;
        }
        else
        {
            _fallBack.Free(mem, size);
        }
    }
    MemAllocSA() = default;
    void Init(void* mem, int size) { _mem = mem, _size = size; }
    ~MemAllocSA() { Clear(); }

    void Clear()
    {
        if (_mem)
        {
            // only non-allocated memory may be released
            DoAssert(_size >= 0);
            _mem = nullptr;
            _size = 0;
        }
    }

    void Unlink(void* mem)
    {
        if (_mem == mem)
        {
            Fail("Unable to unlink");
        }
    }
};

template <class Type>
class MStorage : public MemAllocS
{
  public:
    MemAllocS* Init(int size)
    {
        const int MinSize = 32; // must be higher or equal MinGrow from AutoArray
        if (size < MinSize)
        {
            size = MinSize;
        }
        return InitStorage(size * sizeof(Type));
    }
};

} // namespace Poseidon::Foundation

