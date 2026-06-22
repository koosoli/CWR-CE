#ifndef _MEMHEAP_HPP
#define _MEMHEAP_HPP


#include <Poseidon/Foundation/Types/Pointers.hpp>

#include <Poseidon/Foundation/Containers/CacheList.hpp>
#include <Poseidon/Foundation/Containers/List.hpp>

#include <Poseidon/Foundation/Memory/MemGrow.hpp>

#include <Poseidon/Foundation/Threads/MultiSync.hpp>

#define FreeLink CLTLink<2> // 8B
#define BusyLink CLTLink<1> // 8B

#define INDIRECT 1

#include <Poseidon/Foundation/Memory/FastAlloc.hpp>
#include <Poseidon/Foundation/Memory/MemFreeReq.hpp>


namespace Poseidon::Foundation
{
typedef size_t MemSize;
const int DefaultAlign=16;

void MemoryErrorMalloc( int size );

// very simple chunk based allocation code
// no reference counting, no chunk restoring
enum {InfoAllocChunkSize=8*1024-16};

class InfoAlloc
{

	public:
	enum {chunkSize=InfoAllocChunkSize};

	InfoAlloc(size_t n);
	~InfoAlloc();

	void *Alloc( size_t n );
	void Free( void*pAlloc );
	void Destruct(){FreeChunks();}

	size_t ItemSize() const {return esize;}
	static int ChunkSize() {return sizeof(Chunk);}

	protected:
	struct Chunk;
	struct Link
	{
		Link *next;
		Chunk *chunk; // which chunk is it in
	};
	struct Chunk
	{
		enum {size = chunkSize-2*sizeof(void *)};
		FastAlloc *allocator; // which allocator this chunk serves for
		Chunk *next;
		char mem[size];
	};

	// no free items - add new chunk
	void Grow();

	// parent memory manager
	Chunk *NewChunk();
	void DeleteChunk( Chunk *chunk );

	void FreeChunks();

	Chunk *chunks;
	unsigned int esize;
	Link *head;
};


struct BusyBlock: public BusyLink,public FreeLink
{
	char *_memory; // memory position

	bool IsFree() const
	{
		return FreeLink::next!=nullptr;
	}

private:
	void* operator new[]   (size_t n);
	void  operator delete[](void* ptr);
public:
	void *operator new( size_t n, InfoAlloc &info ){return info.Alloc(n);}
	void *operator new( size_t n, InfoAlloc &info, const char *file, int line ){return info.Alloc(n);}
	void operator delete( void* ptr, InfoAlloc &info ){info.Free(ptr);}
};

typedef BusyBlock FreeBlock;

typedef CLList<BusyBlock,BusyLink> BlockList;

#define NotEndOf(Type,item,list) (Type *)(item)!=(Type *)&(list)

class MemHeap;

class FreeList: public CLList<FreeBlock,FreeLink>
{
	public:
	MemSize MaxFreeLeft( const MemHeap *heap ) const;
	MemSize TotalFreeLeft( const MemHeap *heap ) const;
	int CountFreeLeft() const;
};

class MemoAlloc;

static const int allocSizes[]=
{
	8, 16,
	32, 32+16,
	64, 64+32,
	128, 128+64,
	256,
};

const int nAllocSlots=sizeof(allocSizes)/sizeof(*allocSizes);


// Reserve-and-commit heap: small blocks via FastAlloc slabs, large via a best-fit free list.
class MemHeap: public RefCount
{
	private:
	mutable bool _destruct;

	private:
	MemGrow _memBlocks; // data area

	BlockList _blockList;

	FreeList _freeList;

	int _nItemsFree;

	MemSize _align;
	MemSize _minAlloc;
	const char *_name;
	MemHeap **_lateDestruct;
	MemoryFreeOnDemandList _freeOnDemand;
	
	MemoAlloc *_smallAllocs[nAllocSlots];
	int _allocStats[nAllocSlots+1];
	InfoAlloc _infoAlloc;

	private:
	MemSize AlignSize( MemSize value )
	{
		return (value+_align-1)&~(_align-1);
	}
	MemSize AlignSize( MemSize value, int align )
	{
		return (value+align-1)&~(align-1);
	}

	void InitFastAllocs();
	void DestructFastAllocs();

	void DoConstruct(MemHeap **lateDestructPointer);
	void DoConstruct
	(
		const char *name, MemSize size, MemSize align,
		MemHeap **lateDestructPointer
	);
	void DoDestruct();
		
	public:

	MemHeap
	(
		const char *name, MemSize size, MemSize align=DefaultAlign,
		MemHeap **lateDestructPointer = nullptr
	);
	MemHeap
	(
		MemHeap **lateDestructPointer = nullptr
	);
	void Init
	(
		const char *name, MemSize size,
		MemSize align=DefaultAlign, MemHeap **lateDestructPointer = nullptr
	)
	{
		DoDestruct();
		DoConstruct(name,size,align,lateDestructPointer);
	}
	void Clear() {DoDestruct();}
	~MemHeap();

	size_t FreeOnDemand(size_t size);
	size_t FreeOnDemandAll();
	void RegisterFreeOnDemand(IMemoryFreeOnDemand *object);

	protected:
	FreeBlock *FindBest( MemSize size );

	public:
	void *Alloc( MemSize size, int aligned=-1 );
	void Free( void *pos );
	void CleanUp();

	bool IsFromHeap( void *pos );

	int BlockSize( BusyBlock *cur ) const
	{
		BusyBlock *next=_blockList.Advance(cur);
		char *thisBeg=cur->_memory;
		char *nextBeg=NotEndOf(BusyLink,next,_blockList) ? next->_memory : (char *)_memBlocks.Data()+_memBlocks.Size();
		return nextBeg-thisBeg;
	}

	void *Memory() const {return _memBlocks.Data();}
	MemSize Size() const {return _memBlocks.Size();}

	MemSize MaxFreeLeft() const;
	MemSize TotalFreeLeft() const;
	int CountFreeLeft() const;

	MemSize TotalAllocated() const;
	MemSize TotalCommited() const;
	bool Check() const {return true;}

	void LogAllocStats() const;

	// MemHeap must use customized new,
	// because global new probably uses MemHeap
	void *operator new( size_t size )
	{
		void *ret=malloc(size);
		if( !ret ) MemoryErrorMalloc(size);
		return ret;
	}
	void operator delete( void *mem ) {free(mem);}

	private:
	MemHeap( const MemHeap &src );
	void operator = ( const MemHeap &src );
};


#define SCOPE_LOCK() ScopeLock<CriticalSection> lock(_lock)

class MemHeapLocked: private MemHeap
{
	typedef MemHeap base;

	mutable CriticalSection _lock; // synchronize multithread access

	public:
	// (1) to create use Ref<MemHeapLocked> heap = new MemHeapLocked()
	// (2) to initialize use Init
	//     pass heap name and heap maximum size
	//     for main application heap limit should be something at least 512 MB
	//     for small auxiliary heaps 8..32 MB may be reasonable
	// (3) when the heap is no longer needed, use heap.Free()

	void Init( const char *name, MemSize size, MemSize align=DefaultAlign )
	{
		base::Init(name,size,align);
	}
	void *Alloc( MemSize size, int aligned = -1);
	void Free( void *mem );
	void CleanUp();

	void *operator new( size_t size )
	{
		void *ret=malloc(size);
		if( !ret ) MemoryErrorMalloc(size);
		return ret;
	}
	void *operator new( size_t size, const char *file, int line )
	{
		void *ret=malloc(size);
		if( !ret ) MemoryErrorMalloc(size);
		return ret;
	}
	void operator delete( void *mem ) {free(mem);}

	int AddRef() const {return base::AddRef();}
        int Release() const {return base::Release();}

	void *Memory() const {SCOPE_LOCK();return base::Memory();}
	MemSize Size() const {SCOPE_LOCK();return base::Size();}

	MemSize MaxFreeLeft() const {SCOPE_LOCK();return base::MaxFreeLeft();}
	MemSize TotalFreeLeft() const{SCOPE_LOCK();return base::TotalFreeLeft();}
	int CountFreeLeft() const{SCOPE_LOCK();return base::CountFreeLeft();}

	MemSize TotalAllocated() const{SCOPE_LOCK();return base::TotalAllocated();}
	MemSize TotalCommited() const{SCOPE_LOCK();return base::TotalCommited();}

	void LogAllocStats() const{SCOPE_LOCK();base::LogAllocStats();}

	size_t FreeOnDemand(size_t size){SCOPE_LOCK();return base::FreeOnDemand(size);}
	size_t FreeOnDemandAll(){SCOPE_LOCK();return base::FreeOnDemandAll();}
	void RegisterFreeOnDemand(IMemoryFreeOnDemand *object)
	{
		SCOPE_LOCK();base::RegisterFreeOnDemand(object);
	}
};


} // namespace Poseidon::Foundation

#endif
