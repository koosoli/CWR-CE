#include <Poseidon/Core/Application.hpp>

#pragma optimize("t", on)

#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Foundation/Memory/MemGrow.hpp>
#include <Poseidon/Foundation/Memory/MemHeap.hpp>

#define CHECK 1

// we want Memory Heap to deallocate last
#pragma warning(disable : 4074)
#pragma init_seg(compiler)

// CWRFrameFunctions lives in this TU because LogF is used during MemHeap
// construction and must be defined in the compiler init_seg.

#include <Poseidon/Foundation/Platform/AppFrameExt.hpp>
#include <Poseidon/Foundation/Math/Statistics.hpp>

namespace Poseidon::Foundation
{
static CWRFrameFunctions GCWRFrameFunctions INIT_PRIORITY_URGENT;
AppFrameFunctions* CurrentAppFrameFunctions = &GCWRFrameFunctions;

#include <Poseidon/Foundation/Memory/Memcheck.hpp>

void MemoryFootprint() {}
#define ReportMemory() \
    GMemFunctions->ReportMemory() {}

#define CountNew(file, line, size)

static bool MemoryErrorState = false;

void MemoryErrorReported()
{
    MemoryErrorState = true;
}

void MemoryError(int size);

int MemoryFreeBlocks();

class MemHeap;

extern MemHeap* BMemPtr;

typedef FreeBlock* HeaderType;

InfoAlloc::InfoAlloc(size_t n) : esize(n < sizeof(Link*) ? sizeof(Link*) : n), head(nullptr), chunks(nullptr)
{
    const int alignMem = esize >= 16 ? 16 : 8;
    esize = (esize + alignMem - 1) & ~(alignMem - 1);
}

InfoAlloc::~InfoAlloc() {}

InfoAlloc::Chunk* InfoAlloc::NewChunk()
{
    return (Chunk*)GlobalAlloc(GMEM_FIXED, sizeof(Chunk));
}

void InfoAlloc::DeleteChunk(InfoAlloc::Chunk* chunk)
{
    GlobalFree((HGLOBAL)chunk);
}

void* InfoAlloc::Alloc(size_t n)
{
    PoseidonAssert(n <= esize);
    if (head == nullptr)
    {
        Grow();
    }
    Link* p = head;

    head = p->next;

    return p;
}

void InfoAlloc::Free(void* pAlloc)
{
    Link* p = static_cast<Link*>(pAlloc);
    p->next = head;
    head = p;
}

void InfoAlloc::Grow()
{
    Chunk* n = NewChunk();

    n->next = chunks;
    chunks = n;

    char* start = n->mem;

    int chSize = Chunk::size;
    const int nelem = chSize / esize;

    char* last = &start[(nelem - 1) * esize];
    for (char* p = start; p < last; p += esize)
    {
        Link* link = reinterpret_cast<Link*>(p);

        link->next = reinterpret_cast<Link*>(p + esize);
        link->chunk = n;
    }
    Link* lastLink = reinterpret_cast<Link*>(last);
    lastLink->next = nullptr;
    lastLink->chunk = n;
    head = reinterpret_cast<Link*>(start);
}

void InfoAlloc::FreeChunks()
{
    Chunk* n = chunks;
    while (n)
    {
        Chunk* p = n;
        n = n->next;
        DeleteChunk(p);
    }
    chunks = nullptr;
    head = nullptr;
}

void MemHeap::DoConstruct(MemHeap** lateDestructPointer)
{
    InitFastAllocs();
    _name = "No";
    _lateDestruct = lateDestructPointer;
}

void MemHeap::DoConstruct(const char* name, MemSize size, MemSize align, MemHeap** lateDestructPointer)
{
    InitFastAllocs();

    // LOG_DEBUG unavailable here — called from static initializer before memory system is ready.
    _name = name;

    _destruct = false;

    _align = align;
    size = AlignSize(size);
    _minAlloc = AlignSize(_minAlloc);
    _memBlocks.Reserve(size);

    char* startFree = (char*)_memBlocks.Data();
    int sizeFree = _memBlocks.Size();

    int skip = _align - sizeof(HeaderType);
    if (skip > 0 && skip != _align)
    {
        startFree += skip;
        sizeFree -= skip;
    }

    FreeBlock* allFree = new (_infoAlloc) FreeBlock;
    allFree->_memory = startFree;

    _freeList.Insert(allFree);
    _blockList.Insert(allFree);

    _nItemsFree = 1;
    _lateDestruct = lateDestructPointer;
}

void MemHeap::DoDestruct()
{
    DestructFastAllocs();

    LOG_DEBUG(Memory, "Destruct memory heap {}", _name);

    _blockList.Clear();
    _freeList.Clear();
    _memBlocks.Clear();
    _infoAlloc.Destruct();
    _nItemsFree = 0;
    if (_lateDestruct)
    {
        PoseidonAssert(_lateDestruct == &BMemPtr);
        *_lateDestruct = nullptr; // nothing to diagnose - memory is gone
        Log("**** All memory released - memory checker deinitialized *****");
    }
}

void MemHeap::LogAllocStats() const
{
    StatisticsByName stats;
    for (BusyBlock* block = _blockList.First(); block; block = _blockList.Next(block))
    {
        if (block->IsFree())
        {
            continue;
        }
        int size = BlockSize(block);

        char name[256];
        snprintf(name, sizeof(name), "%08d", size);
        stats.Count(name, 1);
    }
    stats.Report();
}

// Alloc and Free are used only within operator new/delete
// inlining avoids using 1:1 call

#include <string>

MemHeap::~MemHeap()
{
    DoDestruct();
}

size_t MemHeap::FreeOnDemand(size_t size)
{
    return _freeOnDemand.Free(size);
}

size_t MemHeap::FreeOnDemandAll()
{
    return _freeOnDemand.FreeAll();
}

void MemHeap::RegisterFreeOnDemand(IMemoryFreeOnDemand* object)
{
    _freeOnDemand.Register(object);
}

FreeBlock* MemHeap::FindBest(MemSize size)
{
    FreeBlock* best = nullptr;
    size += (MemSize)INT_MIN;
    int bestDiff = INT_MAX + INT_MIN;
    for (FreeBlock* free = _freeList.Start(); NotEndOf(FreeLink, free, _freeList); free = _freeList.Advance(free))
    {
        int freeSize = BlockSize(free);
        int diff = freeSize - size;
        if (diff < bestDiff)
        {
            bestDiff = diff, best = free;
        }
    }
    return best;
}

const int maxSmallAllocSize = 256;

class MemoAlloc : public FastAlloc
{
    MemHeap* _heap;

  public:
    MemoAlloc(MemHeap* heap, size_t n) : FastAlloc(n, "mem", 12), _heap(heap) {}
    ~MemoAlloc() = default;

    void* operator new(size_t size) { return malloc(size); }
    void operator delete(void* mem) { free(mem); }

    Chunk* NewChunk() override;
    void DeleteChunk(Chunk* chunk) override;
};

MemoAlloc::Chunk* MemoAlloc::NewChunk()
{
    return (Chunk*)_heap->Alloc(sizeof(Chunk));
}

void MemoAlloc::DeleteChunk(MemoAlloc::Chunk* chunk)
{
    _heap->Free(chunk);
}

void MemHeap::InitFastAllocs()
{
    for (int i = 0; i < nAllocSlots; i++)
    {
        _smallAllocs[i] = new MemoAlloc(this, allocSizes[i]);
        _allocStats[i] = 0;
    }
}

void MemHeap::DestructFastAllocs()
{
    for (int i = 0; i < nAllocSlots; i++)
    {
        _smallAllocs[i]->~MemoAlloc();
    }
}

const int DefaultMinAlloc = maxSmallAllocSize;

MemHeap::MemHeap(const char* name, MemSize size, MemSize align, MemHeap** lateDestructPointer)
    : _infoAlloc(sizeof(BusyBlock))
{
    _minAlloc = DefaultMinAlloc;
    DoConstruct(name, size, align, lateDestructPointer);
}
MemHeap::MemHeap(MemHeap** lateDestructPointer) : _infoAlloc(sizeof(BusyBlock))
{
    _align = DefaultAlign;
    _minAlloc = DefaultMinAlloc;
    DoConstruct(lateDestructPointer);
}

void* MemHeapLocked::Alloc(MemSize size, int aligned)
{
    SCOPE_LOCK();
    return base::Alloc(size, aligned);
}

void MemHeapLocked::Free(void* mem)
{
    SCOPE_LOCK();
    base::Free(mem);
}

void MemHeapLocked::CleanUp()
{
    SCOPE_LOCK();
    base::CleanUp();
}

void* MemHeap::Alloc(MemSize size, int aligned)
{
    if (size == 0)
    {
        return nullptr;
    }
    if (size >= 256 * 1024 * 1024)
    {
        LOG_ERROR(Memory, "Bad memory allocation, {} B", size);
        return nullptr;
    }

    int needSize = size;
    int allocIndex = nAllocSlots;
    for (int i = 0; i < nAllocSlots; i++)
    {
        if (allocSizes[i] >= needSize)
        {
            allocIndex = i;
            break;
        }
    }
    _allocStats[allocIndex]++;

    if (allocIndex < nAllocSlots)
    {
        MemoAlloc* alloc = _smallAllocs[allocIndex];
        void* mem_with_header = alloc->AllocCounted(needSize);
        if (!mem_with_header)
        {
            return nullptr;
        }
        void* mem = (char*)mem_with_header + sizeof(void*);

        return mem; // skip header
    }
    DoAssert(needSize > maxSmallAllocSize);

    size += sizeof(HeaderType);
    if (aligned < (int)_align)
    {
        aligned = (int)_align;
    }
    size = AlignSize(size, aligned);

    // find best match
    FreeBlock* best = FindBest(size);
    if (!best)
    {
        // if we cannot find any match, try cleaning up small allocators
        LOG_DEBUG(Memory, "{}: cleaning-up", _name);
        CleanUp();
        best = FindBest(size);
        if (!best)
        {
            return nullptr;
        }
    }
    int bestSize = BlockSize(best);
    int bestDiff = bestSize - size;

    char* ret = (char*)((HeaderType*)best->_memory + 1);
    bool ok = _memBlocks.Commit(ret - (char*)_memBlocks.Data() + size);
    if (!ok)
    {
        return nullptr;
    }

    const int minLeft = _minAlloc;
    if (bestDiff <= minLeft)
    {
        _freeList.Delete(best);
        _nItemsFree--;
    }
    else
    {
        FreeBlock* free = new (_infoAlloc) FreeBlock;
        if (!free)
        {
            return nullptr;
        }
        free->_memory = best->_memory + size;

        _freeList.Delete(best);

        _blockList.InsertAfter(best, free);
        _freeList.Insert(free);
    }

    AddRef();

    *(HeaderType*)best->_memory = best;

    if (best->FreeLink::next || best->FreeLink::prev)
    {
        Fail("Alloc: Memory block corrupt.");
    }

    return ret;
}

bool MemHeap::IsFromHeap(void* mem)
{
    return true;
}

void MemHeap::Free(void* mem)
{
    if (!mem)
    {
        return;
    }

    void** smallMem = (void**)mem - 1; // Go back to chunk pointer header
    FastAlloc* alloc = FastAlloc::WhichAllocator(smallMem);
    if (alloc)
    {
        // busyBlock first item is never null

        alloc->FreeCounted(smallMem);
        return;
    }

    HeaderType* header = (HeaderType*)mem - 1;
    FreeBlock* busy = *header;
    if (busy->_memory != (void*)header)
    {
        Fail("Free: Memory block corrupt.");
    }

    Ref<MemHeap> keepAllocated = this;

    if (busy->FreeLink::next || busy->FreeLink::prev)
    {
        Fail("Free: Memory block corrupt.");
        return;
    }

    bool doInsert = true;

    BusyBlock* next = _blockList.Next(busy);
    if (next && next->IsFree())
    {
        FreeBlock* nextCat = (FreeBlock*)next;

        _freeList.Insert(busy);
        _freeList.Delete(nextCat);

        _blockList.Delete(nextCat);

        _infoAlloc.Free(nextCat);

        doInsert = false;
    }

    BusyBlock* prev = _blockList.Prev(busy);
    if (prev && prev->IsFree())
    {
        FreeBlock* prevCat = (FreeBlock*)prev;
        _blockList.Delete(busy);
        // if the block should not be inserted into the free list
        // it is already inserted and must be deleted
        if (!doInsert)
        {
            _freeList.Delete(busy);
            _nItemsFree--;
        }
        doInsert = false;

        _infoAlloc.Free(busy);

        busy = prevCat;
    }

    if (doInsert)
    {
        _freeList.Insert(busy);
        _nItemsFree++;
    }

    Release();
}

void MemHeap::CleanUp()
{
    for (int i = 0; i < nAllocSlots; i++)
    {
        MemoAlloc* ma = _smallAllocs[i];
        PoseidonAssert(ma);
        if (!ma)
        {
            continue;
        }
        ma->CleanUp();
    }
}

MemSize FreeList::MaxFreeLeft(const MemHeap* heap) const
{
    int max = 0;
    for (FreeBlock* free = Start(); NotEnd(free); free = Advance(free))
    {
        int size = heap->BlockSize(free);
        if (max < size)
        {
            max = size;
        }
    }
    return max;
}

MemSize FreeList::TotalFreeLeft(const MemHeap* heap) const
{
    MemSize total = 0;
    for (FreeBlock* free = Start(); NotEnd(free); free = Advance(free))
    {
        int size = heap->BlockSize(free);
        total += size;
    }
    return total;
}

MemSize MemHeap::MaxFreeLeft() const
{
    int max = 0;
    int val;
    val = _freeList.MaxFreeLeft(this);
    if (max < val)
    {
        max = val;
    }
    return max;
}
MemSize MemHeap::TotalFreeLeft() const
{
    int sum = 0;
    sum += _freeList.TotalFreeLeft(this);
    return sum;
}
MemSize MemHeap::TotalAllocated() const
{
    MemSize free = TotalFreeLeft();
    return _memBlocks.Size() - free;
}
MemSize MemHeap::TotalCommited() const
{
    return _memBlocks.GetCommited();
}

int MemHeap::CountFreeLeft() const
{
    return _nItemsFree;
}

#define MemHeapType MemHeap

class RefHeap
{
    Ref<MemHeapType> _memory;

  public:
    operator MemHeapType*() { return _memory; }
    MemHeapType* operator->() { return _memory; }

    RefHeap(int sizeMB = 256);
    ~RefHeap();
};

DWORD FindHeapSizeLimitKB()
{
    DWORD limit = 512;
    DWORD maxmem = 0;
    char message[256];

    // experimental support for special -maxmem argument with 2GB limit
    // NOTE: Cannot use std::string here - this runs during static initialization!
    for (int i = 1; i < __argc; ++i)
    {
        const char* arg = __argv[i];
        size_t argLen = strlen(arg);

        if (argLen > 8 && strncmp(arg, "-maxmem=", 8) == 0)
        {
            const char* numberStr = arg + 8;

            char* end;
            unsigned long value = strtoul(numberStr, &end, 10);

            // accept only a fully-parsed value under the 2 GB ceiling
            if (*end == '\0' && value < 2048)
            {
                maxmem = static_cast<DWORD>(value);
            }
        }
    }

    if (maxmem > 0)
    {
        snprintf(message, sizeof(message),
                 "-maxmem=%d loaded. Using -maxmem is highly experimental. Values above 512MB could cause issues!",
                 maxmem);
        fprintf(stderr, "WARNING: %s\n", message);
        limit = maxmem;
    }

    return limit * 1024;
}

int DetectHeapSizeMB()
{
    // note: memory allocation is not available yet, command line arguments are not parsed yet
    MEMORYSTATUS memstat;
    memstat.dwLength = sizeof(memstat);
    GlobalMemoryStatus(&memstat);
    DWORD maxSizeKB = memstat.dwTotalVirtual / 1024;
    // leave space for the OS and other runtime allocations
    const DWORD memLimitKB = memstat.dwTotalPageFile / 1024 + memstat.dwTotalPhys / 1024;
    // never reserve less than 512 MB — that floor is known to work well
    const DWORD minReservedKB = 512 * 1024;
    const DWORD heapSizeLimitKB = FindHeapSizeLimitKB();

    if (maxSizeKB > memLimitKB)
    {
        maxSizeKB = memLimitKB;
    }
    if (maxSizeKB > heapSizeLimitKB)
    {
        maxSizeKB = heapSizeLimitKB;
    }

    if (maxSizeKB < minReservedKB)
    {
        maxSizeKB = minReservedKB;
    }

    // Cannot use LOG_DEBUG here — memory allocator is not available yet during static init.

    return maxSizeKB / 1024;
}

RefHeap BMemory(DetectHeapSizeMB()) INIT_PRIORITY_URGENT;

MemHeapType* BMemPtr = BMemory;

RefHeap::RefHeap(int sizeMB)
{
    const int align = maxSmallAllocSize;
    _memory = new MemHeapType("Default", sizeMB * 1024 * 1024, align, &BMemPtr);
}

RefHeap::~RefHeap()
{
    LOG_DEBUG(Memory, "Memory static ref deallocated.");
    if (_memory && _memory->TotalAllocated() > 0)
    {
        LOG_DEBUG(Memory, "  {} KB not deallocated", _memory->TotalAllocated() / 1024);
    }
}

void MemoryInit() {}

void MemoryCleanUp()
{
    if (BMemPtr)
    {
        // force all allocators to clean up
        FastCAlloc::CleanUpAll();
        BMemPtr->CleanUp();
    }
}

void MemoryDone()
{
    // all FastAlloc should be cleaned up now
    // SmallAlloc probably are not

    if (BMemPtr)
    {
        BMemPtr->CleanUp();
    }

    if (BMemPtr)
    {
        RptF("Memory should be free now.");
        RptF("  %d B not deallocated", BMemPtr->TotalAllocated());

#if CHECK
        ReportAllocated();
#endif
    }
    else
    {
        LOG_DEBUG(Memory, "Memory is free now.");
    }
}

class MemTableFunctions : public MemFunctions
{
  public:
    void* New(size_t size) override;
    void* New(size_t size, const char* file, int line);
    void Delete(void* mem) override;
    void Delete(void* mem, const char* file, int line);

    // base of the allocation space; null when unknown/undefined
    void* HeapBase() override
    {
        if (!BMemory)
        {
            return nullptr;
        }
        return BMemory->Memory();
    }

    // approximate committed bytes
    size_t HeapUsed() override
    {
        if (!BMemory)
        {
            return 0;
        }
        return BMemory->TotalAllocated();
    }

    // approximate free-block count (fragmentation signal)
    int FreeBlocks() override
    {
        if (!BMemory)
        {
            return 0;
        }
        return BMemory->CountFreeLeft();
    }

    // approximate allocated-block count (fragmentation signal)
    int MemoryAllocatedBlocks() override
    {
        if (!BMemory)
        {
            return 0;
        }
        return BMemory->RefCounter();
    }

    void Report()
    {
        if (!BMemory)
        {
            return;
        }
        int alloc = BMemory->TotalAllocated();
        int commit = BMemory->TotalCommited();
        LOG_DEBUG(Memory, "Total allocated {}, commited {}", alloc, commit);
    }

    bool CheckIntegrity() override { return BMemory->Check(); }
    // when set, the application should limit memory allocation as much as possible
    bool IsOutOfMemory() override { return MemoryErrorState; }

    void CleanUp() override { BMemory->CleanUp(); }
};

MemTableFunctions OMemTableFunctions INIT_PRIORITY_URGENT;

void RegisterMemoryFreeOnDemand(IMemoryFreeOnDemand* object)
{
    DoAssert(BMemory);
    if (BMemory)
    {
        BMemory->RegisterFreeOnDemand(object);
    }
}

size_t MemoryFreeOnDemand(size_t size)
{
    return BMemory->FreeOnDemand(size);
}

void MemoryError(int size)
{
    MemoryErrorReported();
    ErrorMessage("Out of reserved memory (%d KB).\n"
                 "Code change required (current limit %d KB).\n"
                 "Total free %d KB\n"
                 "Free blocks %d, Max free size %d KB",
                 size / 1024, BMemory->Size() / 1024, BMemory->TotalFreeLeft() / 1024, BMemory->CountFreeLeft(),
                 BMemory->MaxFreeLeft() / 1024);
}

void MemoryErrorMalloc(int size)
{
    ErrorMessage("Out of Win32 memory (%d KB).\n", size);
}

// Guard against allocations before main() - helps detect SIOF issues
static bool g_MemorySystemReady = false;

void SetMemorySystemReady(bool ready)
{
    g_MemorySystemReady = ready;
}

inline void* ActualAlloc(size_t size)
{
// Detect allocations during static initialization (before WinMain)
// This helps identify Static Initialization Order Fiasco issues
#if _DEBUG
    if (!g_MemorySystemReady && size > 0)
    {
        // Allocation happening before memory system is initialized!
        // This is usually a static constructor using RString/containers
        //
        // EXCEPTION: CLI11 library has safe static initialization of string constants
        // These allocations are harmless and happen before main() intentionally
        // We allow small allocations (<256 bytes) which are typical for CLI11's internal strings
        if (size >= 256)
        {
            OutputDebugStringA("ERROR: Memory allocation before WinMain!\n");
            OutputDebugStringA("This indicates a Static Initialization Order Fiasco.\n");
            OutputDebugStringA("Check the call stack for static constructors.\n");
            __debugbreak();
        }
    }
#endif

    void* mem = BMemory->Alloc(size);
    if (!mem && size)
    {
        // try to free some memory and retry
        for (;;)
        {
            size_t freed = BMemory->FreeOnDemand(size);
            if (freed == 0)
            {
                // nothing left to free — the request cannot be satisfied
                MemoryError(size);
                return nullptr;
            }
            void* mem = BMemory->Alloc(size);
            if (mem)
            {
                return mem;
            }
            // freed some memory but still not enough — keep freeing until
            // the request succeeds or there is nothing left to free
        }
    }
    return mem;
}

inline void ActualFree(void* mem)
{
    BMemory->Free(mem);
}

void* MemTableFunctions::New(size_t size)
{
    CountNew("nofile", 0, size);
#if CHECK
    size = PrepareAlloc(size);
#endif
    void* ret = ActualAlloc(size);
#if CHECK
    ret = FinishAlloc(ret, size);
#endif
    return ret;
}

void* MemTableFunctions::New(size_t size, const char* file, int line)
{
    CountNew(file, line, size);
#if CHECK
    size = PrepareAlloc(size);
#endif
    void* ret = ActualAlloc(size);
#if CHECK
    ret = FinishAlloc(ret, size, file, line);
#endif
    return ret;
}

void MemTableFunctions::Delete(void* mem)
{
    if (!mem)
    {
        return;
    }
#if CHECK
    mem = PrepareFree(mem);
#endif
    ActualFree(mem);
}
void MemTableFunctions::Delete(void* mem, const char* file, int line)
{
    if (!mem)
    {
        return;
    }
#if CHECK
    mem = PrepareFree(mem);
#endif
    ActualFree(mem);
}

//  Net-heap support:

// mt-safe heap, destructed as late as possible.
RefD<MemHeapLocked> safeHeap INIT_PRIORITY_URGENT;

#define NET_HEAP_SIZE 128 // MB

class MemFunctionsSafe : public MemFunctions
{
  private:
    bool Create();
    void Destroy();

  public:
    void* New(size_t size) override;
    void* New(size_t size, const char* file, int line);
    void Delete(void* mem) override;
    void Delete(void* mem, const char* file, int line);

    void CleanUp() override { BMemory->CleanUp(); }
};

MemFunctionsSafe SafeHeapFunctions INIT_PRIORITY_URGENT;

// Global memory function pointers — must live in the compiler segment so they
// initialize before any static RString object.
MemFunctions* GMemFunctionsPtr = &OMemTableFunctions;
MemFunctions* GSafeMemFunctionsPtr = &SafeHeapFunctions;

bool MemFunctionsSafe::Create()
{
    if (safeHeap)
    {
        return true;
    }
    safeHeap = new MemHeapLocked;
    safeHeap->Init("SafeHeap", NET_HEAP_SIZE << 20);
    return true;
}

void MemFunctionsSafe::Destroy()
{
    if (!safeHeap)
    {
        return;
    }
    safeHeap = nullptr;
}

void* MemFunctionsSafe::New(size_t size)
{
    if (!safeHeap)
    {
        Verify(Create());
    }
    void* mem;
    mem = safeHeap->Alloc(size);
    if (!mem)
    {
        safeHeap->FreeOnDemand(size);
        mem = safeHeap->Alloc(size);
    }
    DoAssert(mem);
    return mem;
}
void* MemFunctionsSafe::New(size_t size, const char* file, int line)
{
    return New(size);
}
void MemFunctionsSafe::Delete(void* mem)
{
    PoseidonAssert(safeHeap);
    safeHeap->Free(mem);
}
void MemFunctionsSafe::Delete(void* mem, const char* file, int line)
{
    Delete(mem);
}

// Global operator new/delete live in globalOperators.cpp; keeping them out of this
// TU avoids #pragma init_seg(compiler) interference with BMemory initialization.

#include <Poseidon/Foundation/Framework/NetLog.hpp>

} // namespace Poseidon::Foundation
