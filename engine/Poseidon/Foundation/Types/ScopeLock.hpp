#pragma once

// Locks the given object for this object's lifetime.

namespace Poseidon::Foundation
{
template<class Lock>
class ScopeLock
{
	protected:
	const Lock *_lock;
	public:
	ScopeLock( const Lock &lock ) {_lock=&lock;_lock->Lock();}
	~ScopeLock() {_lock->Unlock();}
};

// Unlocks the given object for this object's lifetime (inverse of ScopeLock).
template<class Lock>
class ScopeUnlock
{
	protected:
	Lock *_lock;
	public:
	// const_cast keeps the const& API while storing a mutable lock pointer
	ScopeUnlock( const Lock &lock ) {_lock=const_cast<Lock*>(&lock);_lock->Unlock();}
	~ScopeUnlock() {_lock->Lock();}
};

} // namespace Poseidon::Foundation


using ::Poseidon::Foundation::ScopeLock;
using ::Poseidon::Foundation::ScopeUnlock;
