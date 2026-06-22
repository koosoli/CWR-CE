#include <Poseidon/Core/TaskPool.hpp>

#include <enkiTS/TaskScheduler.h>

#include <algorithm>
#include <cassert>
#include <stddef.h>
#include <utility>

// enki's WaitforTask synchronises workers and the waiter through its own atomics/semaphores, which
// ThreadSanitizer does not model — so it reports false races between a partition's writes and the
// main thread's post-Wait reads (e.g. LandCache::Fill's Pass 4 over the generated segments). Teach
// TSan the real happens-before with explicit acquire/release on the task object. No-op off TSan.
#if defined(__SANITIZE_THREAD__)
#define POSEIDON_TSAN_ENABLED 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define POSEIDON_TSAN_ENABLED 1
#endif
#endif

#ifdef POSEIDON_TSAN_ENABLED
#include <sanitizer/tsan_interface.h>
#define POSEIDON_TSAN_RELEASE(addr) __tsan_release(addr)
#define POSEIDON_TSAN_ACQUIRE(addr) __tsan_acquire(addr)
#else
#define POSEIDON_TSAN_RELEASE(addr) ((void)0)
#define POSEIDON_TSAN_ACQUIRE(addr) ((void)0)
#endif

namespace Poseidon
{

namespace
{

// Adapter: enki::ITaskSet wants a class with an ExecuteRange method.
// `std::function` lifetime is borrowed from the ParallelFor caller —
// the call blocks until WaitforTask returns, so the function outlives
// every worker invocation.
class RangeTask : public enki::ITaskSet
{
  public:
    RangeTask(uint32_t count, const std::function<void(uint32_t, uint32_t)>& work) : enki::ITaskSet(count), _work(work)
    {
    }

    void ExecuteRange(enki::TaskSetPartition range, uint32_t /*threadnum*/) override
    {
        POSEIDON_TSAN_ACQUIRE(this); // see the dispatcher's pre-AddTaskSetToPipe writes
        _work(range.start, range.end);
        POSEIDON_TSAN_RELEASE(this); // publish this partition's writes to the waiter
    }

  private:
    const std::function<void(uint32_t, uint32_t)>& _work;
};

// Adapter for fire-and-forget Submit.  Owns the callable by value so
// the task survives across the (asynchronous) Submit-to-Execute gap.
// `TaskPool::_pending` owns one of these per in-flight submission and
// reaps via `GetIsComplete()` on the next Submit (and at shutdown).
struct FireAndForgetTask : public enki::ITaskSet
{
    std::function<void()> work;

    explicit FireAndForgetTask(std::function<void()> w) : enki::ITaskSet(1), work(std::move(w)) {}

    void ExecuteRange(enki::TaskSetPartition /*range*/, uint32_t /*threadnum*/) override { work(); }
};

// Lifetime-of-process slot for the optional global pool.  Tests
// construct their own `TaskPool` directly and bypass this entirely.
std::unique_ptr<TaskPool> g_globalPool;

} // namespace

TaskPool::TaskPool(uint32_t threadCount) : _scheduler(std::make_unique<enki::TaskScheduler>())
{
    if (threadCount == 0)
    {
        _scheduler->Initialize();
    }
    else
    {
        _scheduler->Initialize(threadCount);
    }
}

TaskPool::~TaskPool()
{
    // enkiTS requires WaitforAllAndShutdown before destruction or
    // outstanding worker threads observe freed scheduler state.
    if (_scheduler)
    {
        _scheduler->WaitforAllAndShutdown();
    }
    // After WaitforAllAndShutdown every pending task has finished —
    // safe to delete them all unconditionally.
    for (void* p : _pending)
    {
        delete static_cast<FireAndForgetTask*>(p);
    }
    _pending.clear();
}

void TaskPool::ParallelFor(uint32_t count, const std::function<void(uint32_t, uint32_t)>& work)
{
    if (count == 0)
    {
        return;
    }
    if (count == 1)
    {
        // Single-item shortcut: avoids scheduling overhead for the
        // degenerate case landscape's nMiss==1 path already hand-coded.
        work(0, 1);
        return;
    }

    std::lock_guard<std::mutex> lock(_submitMutex);
    RangeTask task(count, work);
    POSEIDON_TSAN_RELEASE(&task); // publish the caller's prior writes to the workers
    _scheduler->AddTaskSetToPipe(&task);
    _scheduler->WaitforTask(&task);
    POSEIDON_TSAN_ACQUIRE(&task); // acquire every partition's writes before returning to the caller
}

void TaskPool::Submit(std::function<void()> work)
{
    std::lock_guard<std::mutex> lock(_submitMutex);
    GcCompletedPending();
    auto* task = new FireAndForgetTask(std::move(work));
    _pending.push_back(task);
    _scheduler->AddTaskSetToPipe(task);
}

void TaskPool::GcCompletedPending()
{
    // Caller holds _submitMutex.  enki::ICompletable::GetIsComplete()
    // is safe to query from any thread that submitted (and is the
    // standard way to poll fire-and-forget tasks).
    size_t writeIdx = 0;
    for (size_t i = 0; i < _pending.size(); ++i)
    {
        auto* task = static_cast<FireAndForgetTask*>(_pending[i]);
        if (task->GetIsComplete())
        {
            delete task;
        }
        else
        {
            _pending[writeIdx++] = _pending[i];
        }
    }
    _pending.resize(writeIdx);
}

uint32_t TaskPool::ThreadCount() const
{
    return _scheduler ? _scheduler->GetNumTaskThreads() : 0;
}

uint32_t ResolveTaskPoolThreadCount(int cliOverride, unsigned hardwareThreads)
{
    if (cliOverride > 0)
    {
        return static_cast<uint32_t>(cliOverride); // explicit count
    }
    if (cliOverride == 0)
    {
        return 0; // uncapped — 0 makes enkiTS use every logical core
    }
    // auto: cap at the default, but never request more threads than the machine has.
    const unsigned cores = hardwareThreads > 0 ? hardwareThreads : kDefaultMaxTaskThreads;
    return std::min<unsigned>(cores, kDefaultMaxTaskThreads);
}

enki::TaskScheduler* TaskPool::RawScheduler()
{
    return _scheduler.get();
}

TaskPool* GetGlobalTaskPool()
{
    return g_globalPool.get();
}

void InitGlobalTaskPool(uint32_t threadCount)
{
    if (!g_globalPool)
    {
        g_globalPool = std::make_unique<TaskPool>(threadCount);
    }
}

void ShutdownGlobalTaskPool()
{
    g_globalPool.reset();
}

} // namespace Poseidon
