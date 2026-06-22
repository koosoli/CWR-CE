#pragma once

// Lock-tracing (debug only) — uncomment to enable.
// #define LOCK_TRACING

#ifdef LOCK_TRACING

#define LockDecl(lock, descr) lock(__FILE__, __LINE__, descr)
#define LockInit(lock, descr, val) lock(__FILE__, __LINE__, descr, val)
#define LockRegister(lock, descr) lock.registerMe(__FILE__, __LINE__, descr)

#else

#define LockDecl(lock, descr) lock
#define LockInit(lock, descr, val) lock(val)
#define LockRegister(lock, descr)

#endif

#include <Poseidon/Foundation/Types/Pointers.hpp>

#if defined _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <pthread.h>
#include <semaphore.h>
#endif

namespace Poseidon::Foundation
{
#ifndef _WIN32
extern pthread_mutex_t mutexInit;
#endif

class PoCriticalSection : public Poseidon::Foundation::RefCount
{
  protected:
#ifdef _WIN32

    mutable CRITICAL_SECTION cs;

#else

    mutable pthread_mutex_t mutex;

#endif

#ifdef LOCK_TRACING

    // index into the LockInstance array, or -1 if not registered
    mutable int id;

#endif

    // when false, all operations are no-ops
    bool valid;

  public:
    mutable bool error;

    // Recursive: the same thread may re-enter. val=false makes every operation a no-op.
    PoCriticalSection(bool val);

    PoCriticalSection();

#ifdef LOCK_TRACING

    PoCriticalSection(const char* srcFile, int lineNo, const char* descr = nullptr);

    void registerMe(const char* srcFile, int lineNo, const char* descr = nullptr);

#endif

    // Blocks until acquired. Recursive: every enter() needs a matching leave().
    void enter() const;

    inline void Lock() const { enter(); }

    // Non-blocking: enters and returns true if free, returns false without blocking otherwise.
    bool tryEnter() const;

    void leave() const;

    inline void Unlock() const { leave(); }

    ~PoCriticalSection() override;

    static void* operator new(size_t size);

    static void* operator new(size_t size, const char* file, int line);

    static void operator delete(void* mem);

#ifdef __INTEL_COMPILER

    // Intel compiler needs this for exception unwind.
    static void operator delete(void* mem, const char* file, int line);

#endif
};

} // namespace Poseidon::Foundation

