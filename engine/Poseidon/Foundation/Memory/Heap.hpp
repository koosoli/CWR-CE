#pragma once

#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Containers/CacheList.hpp>
#include <Poseidon/Foundation/Memory/FastAlloc.hpp>

// operators required:
// HeapSize must be arithmetic
// Index operator + ( Index, HeapSize )



namespace Poseidon::Foundation
{
template <class Index,class HeapSize>
class Heap
{
	public:
	class HeapItem: public CLDLink
	{
		friend class Heap<Index,HeapSize>;

		protected:
		Index _pos;
		HeapSize _size;

		public:
		Index Memory() const {return _pos;}
		HeapSize Size() const {return _size;}

		USE_FAST_ALLOCATOR;
	};
	
	private:
	CLList<HeapItem> _freeRoot;
	CLList<HeapItem> _busyRoot;
	HeapSize _align;

	Index _pos;
	HeapSize _size;
	HeapSize _totalBusy; // _size-_totalBusy=_totalFree
	
	Index AlignIndex( Index value )
	{
		return (Index)(((intptr_t)value+_align-1)&~(_align-1));
	}
	HeapSize AlignSize( HeapSize value )
	{
		return (value+_align-1)&~(_align-1);
	}

	void DoConstruct( Index pool, HeapSize size, HeapSize align );
	void DoDestruct();
	
	public:
	Heap( Index pool, HeapSize size, HeapSize align )
	{
		DoConstruct(pool,size,align);
	}
	Heap()
	{
		_align=1;
		_pos=0;
		_size=0;
	}
	void Init( Index pool, HeapSize size, HeapSize align )
	{
		DoDestruct();
		DoConstruct(pool,size,align);
	}
	void Release() {DoDestruct();}
	~Heap() {DoDestruct();}

	HeapItem *Alloc( HeapSize size );
	HeapItem *Alloc( Index at, HeapSize size );
	void Free( HeapItem *pos );

	Index Memory() const {return _pos;}
	HeapSize Size() const {return _size;}

	void Move( HeapSize offset ); // be careful with this function

	HeapSize MaxFreeLeft() const;
	HeapSize CalcTotalFreeLeft() const;
	HeapSize CalcTotalBusy() const;
	int CountFreeLeft() const;

	HeapSize TotalFreeLeft() const
	{
		AssertDebug( _size-_totalBusy==CalcTotalFreeLeft() );
		return _size-_totalBusy;
	}
	HeapSize TotalBusy() const
	{
		AssertDebug( _totalBusy==CalcTotalBusy() );
		return _totalBusy;
	}

	bool Check() const;
	NoCopy(Heap)
};


template <class Index, class HeapSize>
void Heap<Index,HeapSize>::DoConstruct( Index pool, HeapSize size, HeapSize align )
{
	_totalBusy=0;
	_align=align;
	// pool must be aligned
	Index alignPool=AlignIndex(pool);
	size-=alignPool-pool;
	pool=alignPool;
	// size must be aligned
	size=size&~(_align-1);
	// setup data members
	HeapItem *allFree=new HeapItem;
	allFree->_pos=pool;
	allFree->_size=size;
	_freeRoot.Insert(allFree);
	_pos=pool;
	_size=size;
}

template <class Index, class HeapSize>
void Heap<Index,HeapSize>::DoDestruct()
{
	HeapItem *item;
	// caution: pointer aliasing - changes root
	// root will change from item to item->next
	while( (item=_freeRoot.First())!=nullptr )
	{
		item->Delete(),delete item;
	}
	while( (item=_busyRoot.First())!=nullptr )
	{
		item->Delete(),delete item;
	}
	_pos=0;
	_size=0;
}


template <class Index, class HeapSize>
typename Heap<Index,HeapSize>::HeapItem *Heap<Index,HeapSize>::Alloc
(
	Index at, HeapSize size
)
{
	// force allocation at given place
	if( size==0 ) return nullptr;
	// align size
	size=AlignSize(size);
	// find best match
	HeapItem *free;
	HeapItem *best=nullptr;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		// select free item containing given block
		if( free->_pos<=at && free->_pos+free->_size>=at+size )
		{
			best=free;
			break;
		}
	}
	if( !best ) return nullptr;
	if( free->_pos==at && free->_size==size ) // exact match
	{
		// delete best from the free list
		best->Delete();
		// insert best into the busy list
		_busyRoot.Insert(best);
		_totalBusy+=best->_size;
		PoseidonAssert( _totalBusy<_size );
		return best;
	}
	else
	{
		// split rest of the item
		Index end=free->_pos+free->_size;
		if( end>at+size )
		{
			HeapItem *rest=new HeapItem;
			rest->_pos=at+size;
			rest->_size=end-rest->_pos;
			_freeRoot.Insert(rest);
		}
		HeapItem *busy=new HeapItem;
		// use last size elements from best
		busy->_pos=at;
		busy->_size=size;
		best->_size=at-best->_pos;
		
		// insert busy into the busy list
		_busyRoot.Insert(busy);
		_totalBusy+=busy->_size;
		PoseidonAssert( _totalBusy<_size );
		return busy;
	}
}

template <class Index, class HeapSize>
typename Heap<Index,HeapSize>::HeapItem *Heap<Index,HeapSize>::Alloc( HeapSize size )
{
	if( size==0 ) return nullptr;
	// align size
	size=AlignSize(size);
	// find best match
	HeapItem *free;
	HeapSize bestDiff=INT_MAX;
	HeapItem *best=nullptr;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		HeapSize diff=free->_size-size;
		if( diff<0 ) continue;
		if( diff<bestDiff )
		{
			best=free;
			bestDiff=diff;
			if( diff==nullptr ) break; // no match could be better
		}
	}
	if( !best )
	{
		return nullptr;
	}
	if( bestDiff==0 ) // exact match
	{
		// delete best from the free list
		best->Delete();
		
		// insert best into the busy list
		_busyRoot.Insert(best);
		_totalBusy+=best->_size;
		PoseidonAssert( _totalBusy<_size );
		return best;
	}
	else
	{
		// split item into two parts
		HeapItem *busy=new HeapItem;
		// use first size elements from best
		busy->_size=size;
		busy->_pos=best->_pos;
		// eat first size elements from best
		best->_size=bestDiff;
		best->_pos+=size;
		
		// insert busy into the busy list
		_busyRoot.Insert(busy);
		_totalBusy+=busy->_size;
		PoseidonAssert( _totalBusy<_size );
		return busy;
	}
}

template <class Index, class HeapSize>
void Heap<Index,HeapSize>::Free( HeapItem *item )
{
	if( !item ) return;
	HeapItem *free;
	_totalBusy-=item->_size;
	PoseidonAssert( _totalBusy>=0 );
	for( free=_freeRoot.First(); free; )
	{
		HeapItem *next=_freeRoot.Next(free);
		// try to find free item just before item
		if( free->_pos+free->_size==item->_pos )
		{
			// prepend free to item
			item->_pos=free->_pos;
			item->_size+=free->_size;
			// delete free
			free->Delete(),delete free;
		}
		// try to find free item just after item
		else if( free->_pos==item->_pos+item->_size )
		{
			// append free to item
			item->_size+=free->_size;
			// delete free
			free->Delete(),delete free;
		}
		free=next;
	}
	// remove item from the busy list
	item->Delete();
	// insert item into the free list
	_freeRoot.Insert(item);
}

template <class Index, class HeapSize>
HeapSize Heap<Index,HeapSize>::MaxFreeLeft() const
{
	HeapSize max=0;
	HeapItem *free;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		if( max<free->_size ) max=free->_size;
	}
	return max;
}
template <class Index, class HeapSize>
HeapSize Heap<Index,HeapSize>::CalcTotalFreeLeft() const
{
	HeapSize total=0;
	HeapItem *free;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		total+=free->_size;
	}
	return total;
}
template <class Index, class HeapSize>
int Heap<Index,HeapSize>::CountFreeLeft() const
{
	int count=0;
	HeapItem *free;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		count++;
	}
	return count;
}

template <class Index, class HeapSize>
HeapSize Heap<Index,HeapSize>::CalcTotalBusy() const
{
	int total=0;
	HeapItem *busy;
	for( busy=_busyRoot.First(); busy; busy=_busyRoot.Next(busy) )
	{
		total+=busy->_size;
	}
	return total;
}

template <class Index, class HeapSize>
void Heap<Index,HeapSize>::Move( HeapSize offset ) // be careful with this function
{
	_pos+=offset; // relocate heap base
	// relocate all heap items
	HeapItem *free,*busy;
	for( free=_freeRoot.First(); free; free=_freeRoot.Next(free) )
	{
		free->_pos+=offset;
	}
	for( busy=_busyRoot.First(); busy; busy=_busyRoot.Next(busy) )
	{
		busy->_pos+=offset;
	}
}

template <class Index, class HeapSize>
bool Heap<Index,HeapSize>::Check() const
{
	HeapItem *item;
	for( item=_freeRoot.First(); item; item=_freeRoot.Next(item) )
	{
		if( item->_pos<_pos ){Fail("Heap check");return false;}
		if( item->_size<0 ){Fail("Heap check");return false;}
		if( item->_size>_size ){Fail("Heap check");return false;}
		if( item->_pos+item->_size>_pos+_size ){Fail("Heap check");return false;}
	}
	for( item=_busyRoot.First(); item; item=_busyRoot.Next(item) )
	{
		if( item->_pos<_pos ){Fail("Heap check");return false;}
		if( item->_size<0 ){Fail("Heap check");return false;}
		if( item->_size>_size ){Fail("Heap check");return false;}
		if( item->_pos+item->_size>_pos+_size ){Fail("Heap check");return false;}
	}
	return true;
}


} // namespace Poseidon::Foundation
