#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>



namespace Poseidon
{
class SmartHandle
{
	int _index;
	int _counter;

	public:
	SmartHandle( int index, int counter )
	:_index(index),_counter(counter)
	{
	}
	SmartHandle():_index(-1),_counter(-1){} // invalid handle
	int Index() const {return _index;}
	int Counter() const {return _counter;}
	bool Valid() const
	{
		return _index!=-1 || _counter!=-1;
	}
	int operator ==( const SmartHandle &with ) const
	{
		return _index==with._index && _counter==with._counter;
	}
};

template <class Type>
class HandleInfo
{
	int _counter;
	Type *_ptr;

	public:
	HandleInfo():_ptr(nullptr),_counter(0){}
	HandleInfo( Type *ptr, int counter ):_ptr(ptr),_counter(counter){}

	void SetCounter( int counter ) {_counter=counter;}
	int Counter() const {return _counter;}

	operator Type *() const {return _ptr;}
	Type * operator -> () const {return _ptr;}
};

// handles used to reference items of handled list can become invalid any time
// operator [] returns nullptr when passed an invalid handles

template <class Type>
class HandledList
{
	// Type should be based on HandleInfo
	AutoArray< HandleInfo<Type> > _data;
	int _counter; // produce unique marks for sounds
	// note: this would cycle after 2^24 uses - hope this will not make problems
	
	protected:
	void CloseIndex( int index );

	private:
	void operator = ( const HandledList &src );
	HandledList( const HandledList &src );

	public:
	~HandledList(){CloseAll();}
	HandledList():_counter(0){}

	int NElements() const {return _data.Size();}
	SmartHandle GetHandle( int index ) const;
	
	SmartHandle NewElement( Type *ptr );
	Type *Element( SmartHandle handle ) const;
	Type *ElementIndex( int index ) const {return _data[index];}
	Type *operator [] ( SmartHandle i ) const {return Element(i);}
	
	void Close( SmartHandle handle );
	void CloseAll();
};

template <class Type>
inline SmartHandle HandledList<Type>::GetHandle( int index ) const
{
	return SmartHandle(index,_data[index].Counter());
}

template <class Type>
SmartHandle HandledList<Type>::NewElement( Type *ptr )
{
	HandleInfo<Type> info(ptr,_counter++);
	int i;
	for( i=0; i<_data.Size(); i++ )
	{
		if( !_data[i] )
		{
			_data[i]=info;
			return GetHandle(i);
		}
	}
	i=_data.Add(info);
	return GetHandle(i);
}

template <class Type>
void HandledList<Type>::CloseIndex( int index )
{
	delete _data[index];
	_data[index]=HandleInfo<Type>();
}

template <class Type>
Type *HandledList<Type>::Element( SmartHandle handle ) const
{
	int index=handle.Index();
	int counter=handle.Counter();
	if( index<0 ) return nullptr; // invalid pointer
	if( _data.Size()<=index ) return nullptr;
	if( _data[index].Counter()!=counter ) return nullptr;
	return _data[index];
}

template <class Type>
void HandledList<Type>::Close( SmartHandle handle )
{
	int index=handle.Index();
	int counter=handle.Counter();
	if
	(
		_data.Size()<=index || !_data[index]
		|| _data[index].Counter()!=counter
	)
	{
		return;
	}
	CloseIndex(index);
}

template <class Type>
void HandledList<Type>::CloseAll()
{
	for( int i=0; i<_data.Size(); i++ )
	{
		if( _data[i] ) CloseIndex(i);
	}
	_data.Clear();
}

template <class Type>
class PriorityList: public HandledList<Type>
{
	protected:
	int _maxElements;

	public:
	// there must be operator < defined for Type
	PriorityList( int maxElements ); // keep maximum number of elements
	SmartHandle NewElement( Type *element ); // may return invalid handle
};


template <class Type>
PriorityList<Type>::PriorityList( int maxElements )
:_maxElements(maxElements)
{
 // keep maximum number of elements
}

template <class Type>
SmartHandle PriorityList<Type>::NewElement( Type *element )
{
	// may return invalid handle
	// if all slots are already used and all current elements have higher priority
	if( this->NElements()>=_maxElements )
	{
		int index;
		int smallest=-1;
		for( index=0; index<this->NElements(); index++ )
		{
			if( !this->ElementIndex(index) ) {smallest=index;goto Empty;}
			if( smallest<0 ) smallest=index;
			else if( *this->ElementIndex(index)<*this->ElementIndex(smallest) ) smallest=index;
		}
		// if even the smallest has higher priority, return Invalid
		PoseidonAssert( smallest>=0 );
		if( *element<*this->ElementIndex(smallest) ) return SmartHandle(); // invalid
		// close smallest
		this->CloseIndex(smallest);
	}
	Empty:
	return HandledList<Type>::NewElement(element);
}
} // namespace Poseidon
