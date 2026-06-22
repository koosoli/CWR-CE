#pragma once

#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#ifdef _WIN32
#include <Poseidon/Foundation/Memory/CheckMem.hpp>
#endif

namespace Poseidon::Foundation
{
#ifdef _WIN32

typedef HANDLE ThreadId;

#define THREAD_PROC_RETURN DWORD
#define THREAD_PROC_MODE WINAPI

#else

typedef pthread_t ThreadId;

#define THREAD_PROC_RETURN void*
#define THREAD_PROC_MODE

#endif

// Create and start a thread running threadProc(arg). id (out) may be null; stackSize 0 means default.
extern bool poThreadCreate(ThreadId* id, long stackSize, THREAD_PROC_RETURN(THREAD_PROC_MODE* threadProc)(void*),
                           void* arg);

// Block until thread id terminates; result (optional) receives its return value.
extern bool poThreadJoin(ThreadId id, THREAD_PROC_RETURN* result);

extern void poThreadId(ThreadId& id);

// Relative priority from -2 (lowest) to 2 (highest).
extern bool poSetPriority(ThreadId id, int delta);

extern bool poSetMyPriority(int delta);

#ifdef _WIN32

class MemAllocSafe
{
  public:
#if ALLOC_DEBUGGER

    static void* Alloc(int& size, const char* file, int line, const char* postfix)
    {
        (void)file;
        (void)line;
        (void)postfix;
        return Poseidon::Foundation::safeNew(size);
    }

#else

    static void* Alloc(int& size) { return Poseidon::Foundation::safeNew(size); }

#endif

    static void Free(void* mem, int size)
    {
        (void)size;
        Poseidon::Foundation::safeDelete(mem);
    }

    static void Free(void* mem) { Poseidon::Foundation::safeDelete(mem); }

    static void Unlink(void* mem) { (void)mem; }
};

#else

#define MemAllocSafe MemAllocD

#endif

// Reference-counted base whose count is guarded by a critical section.
class RefCountSafe
{
  private:
    mutable int _count;

  protected:
    // guards _count; also usable for general per-object locking
    mutable PoCriticalSection lock;

  public:
    RefCountSafe() { _count = 0; }

    // _count is not copied — a copy starts unreferenced.
    RefCountSafe(const RefCountSafe& src)
    {
        (void)src;
        _count = 0;
    }

    void operator=(const RefCountSafe& src)
    {
        (void)src;
        _count = 0;
    }

    virtual ~RefCountSafe();

    inline void enter() const { lock.enter(); }

    inline void leave() const { lock.leave(); }

    int AddRef() const
    {
        enter();
        int result = ++_count;
        leave();
        return result;
    }

    int Release() const
    {
        enter();
        int ret = --_count;
        leave();
        if (ret == 0)
        {
            delete (RefCountSafe*)this;
        }
        return ret;
    }

    int RefCounter() const { return _count; }

    // memory used by this object, excluding the object itself (covered by sizeof)
    virtual double GetMemoryUsed() const;
};

} // namespace Poseidon::Foundation

