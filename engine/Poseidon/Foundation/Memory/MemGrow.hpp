#ifndef _MEMGROW_HPP
#define _MEMGROW_HPP


#include <cstddef>
namespace Poseidon::Foundation
{
class MemGrow
{
	private:
	void *_data;
	size_t _reserved,_commited;
	size_t _pageSize;
	bool _error;

	protected:
	void DoConstruct();
	void DoConstruct( size_t size );
	void DoDestruct();

	private: // no copy
	MemGrow( const MemGrow &src );
	void operator = ( const MemGrow &src );

	public:
	MemGrow(){DoConstruct();}
	MemGrow( size_t size ){DoConstruct(size);}
	~MemGrow(){DoDestruct();}
	void Reserve( size_t size );
	bool Commit( size_t size );
	size_t GetCommited() const {return _commited;}
	size_t GetReserved() const {return _reserved;}
	void Clear(){DoDestruct();}
	void *Data() const {return _data;}
	size_t Size() const {return _reserved;}
};

} // namespace Poseidon::Foundation

#endif
