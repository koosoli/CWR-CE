#ifndef _MEMCHECK_HPP
#define _MEMCHECK_HPP

// memory allocation tracking - find un-freed blocks
#include <Poseidon/Foundation/Containers/CacheList.hpp>
#include <Poseidon/Core/Application.hpp>

// On x64, pointers are 64-bit, so we need more space for the header
#if defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
#define SPARE_DWORD 6  // Need 8 bytes for pointer + 4 for magic + 4 for alignment
#else
#define SPARE_DWORD 4  // 4 bytes pointer + 4 bytes magic on x86
#endif

#define GUARD_SIZE 16

#define GUARD_VAL 0x6a
#define FREE_VAL 0xdd
#define NEW_VAL 0xbb


namespace Poseidon::Foundation
{
class MemoryInfo: public CLDLink
{
	private:
	// record all memory blocks
	void *_mem;
	int _size;
	char _file[128-20];
	int _line;

	public:
	MemoryInfo( void *m, int s )
	{
		_mem=m,_size=s;
		_file[0]=0,_line=0;
	}
	MemoryInfo( void *m, int s, const char *file, int line )
	{
		_mem=m,_size=s;
		strncpy(_file,file,sizeof(_file));
		_file[sizeof(_file)-1]=0;
		_line=line;
	}

	void Report() const
	{
		const int textSize=64;
		char text[textSize+1];
		strncpy(text,(char *)_mem,textSize);
		text[textSize]=0;
		if( textSize>_size ) text[_size]=0;
		if( _file && *_file )
		{
			if( strchr(_file,'.') )
			{
				LOG_DEBUG(Memory, "{}({}): Memory {}:{:6} '{}'",_file,_line,(uintptr_t)_mem,_size,text);
			}
			else
			{
				// _file is probably class name
				LOG_DEBUG(Memory, "Memory {}:{:6}: '{}':{}",fmt::ptr(_mem),_size,_file,_line);
			}
		}
		else
		{
			LOG_DEBUG(Memory, "Memory {}:{:6} '{}'",(uintptr_t)_mem,_size,text);
		}
	}

	bool Valid() const {return _mem!=nullptr && _size>=0;}
	void Invalidate() {_mem=nullptr,_size=-1;}

	void *Addr() const {return _mem;}
	int Size() const {return _size;}
	const char *File() const {return _file;}
	int Line() const {return _line;}

	// override memory allocation
	// this array must not use the resources it monitors
	void *operator new ( size_t size ) {return malloc(size);}
	void operator delete ( void *mem ) {free(mem);}
};

class MemList: public CLList<MemoryInfo>
{
	public:
	void *operator new ( size_t size ) {return malloc(size);}
	void operator delete ( void *mem ) {free(mem);}
};

static MemList *PAllocated=new MemList;

#define Allocated (*PAllocated)



inline void *PrepareFree( void *mem )
{
	int *block=(int *)mem-SPARE_DWORD;
	
	// Read magic number - it's at offset dependent on platform
#if defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
	int magic=block[2];  // After 8-byte pointer
#else
	int magic=block[1];  // After 4-byte pointer
#endif
	
	PoseidonAssert( magic==15879634 );
	if( magic!=15879634 ) return block;
	
	// Read MemoryInfo pointer
	MemoryInfo *info;
#if defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
	info = *(MemoryInfo**)block;  // Read 8-byte pointer
#else
	info = (MemoryInfo*)block[0];  // Read 4-byte pointer cast to int
#endif
	
	PoseidonAssert( info->Valid() );
	if( info->Valid() )
	{
		int fullSize=info->Size()+GUARD_SIZE+SPARE_DWORD*sizeof(int);
		char *guard=(char *)block+fullSize-GUARD_SIZE;
		for( int i=0; i<GUARD_SIZE; i++ )
		{
			if( guard[i]!=GUARD_VAL )
			{
				LOG_ERROR(Memory, "GUARD (after {:x}) Memory changed.",(uintptr_t)mem);
				info->Report();
			}
		}
		
#if defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
		guard = (char *)(block+3);  // After pointer + magic
		for( int i=0; i<(SPARE_DWORD-3)*sizeof(int); i++ )
#else
		guard = (char *)(block+2);
		for( int i=0; i<(SPARE_DWORD-2)*sizeof(int); i++ )
#endif
		{
			if( guard[i]!=GUARD_VAL )
			{
				LOG_ERROR(Memory, "GUARD (before {:x}) Memory changed.",(uintptr_t)mem);
				info->Report();
			}
		}

#if defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
		memset(block+3,GUARD_VAL,sizeof(int)*(SPARE_DWORD-3));
#else
		memset(block+2,GUARD_VAL,sizeof(int)*(SPARE_DWORD-2));
#endif

		// fill invalid memory with FREE_VAL
		info->Delete();
		memset(block,FREE_VAL,fullSize);
		info->Invalidate();
	}
	return block;
}

inline int PrepareAlloc( int size )
{
	return size+sizeof(int)*SPARE_DWORD+GUARD_SIZE; // record memory info pointer
}

inline void *FinishAlloc( void *ret, int size, const char *file="", int line=0 )
{
	if( ret )
	{
		int noGuardSize=size-GUARD_SIZE;
		int origSize=noGuardSize-SPARE_DWORD*sizeof(int);
		int *block=(int *)ret;
		MemoryInfo *info=new MemoryInfo(block+SPARE_DWORD,origSize,file,line);
		Allocated.Insert(info);
		
		// Store MemoryInfo pointer
#if defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
		*(MemoryInfo**)block = info;  // Store 8-byte pointer
		block[2] = 15879634;           // Magic number after pointer
		memset(block+3,GUARD_VAL,sizeof(int)*(SPARE_DWORD-3));
#else
		block[0]=(int)info;            // Store pointer cast to int (x86)
		block[1]=15879634;             // Magic number
		memset(block+2,GUARD_VAL,sizeof(int)*(SPARE_DWORD-2));
#endif
		
		void *guard=(char *)ret+noGuardSize;
		ret=block+SPARE_DWORD;
		memset(ret,NEW_VAL,origSize);
		memset(guard,GUARD_VAL,GUARD_SIZE);
	}
	return ret;
}

inline void ReportAllocated()
{
	for
	(
		MemoryInfo *info=Allocated.First();
		info;
		info=Allocated.Next(info)
	)
	{
		info->Report();
	}
}

} // namespace Poseidon::Foundation

#endif
