#pragma once

#ifdef _WIN32
#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Foundation/Framework/AppFrame.hpp> // ErrorMessage
#else
#include <Poseidon/Foundation/PoseidonPCH.hpp>
#include <Poseidon/Foundation/Threads/PoCritical.hpp>
#endif
#include <Poseidon/Foundation/Types/ScopeLock.hpp>

namespace Poseidon::Foundation
{
#ifdef _WIN32

class SignaledObject
{
	private:
	SignaledObject(const SignaledObject &src);
	void operator =(const SignaledObject &src);

	protected:
	mutable HANDLE _handle;

	public:
	SignaledObject( HANDLE handle ):_handle(handle)
	{
		if( !_handle ) ErrorMessage("Win32 Error: Cannot create MT object.");
	}
	~SignaledObject(){if( _handle ) CloseHandle(_handle),_handle=nullptr;}

	// wait/lock are synonymous
	bool Wait() const {return WaitForSingleObject(_handle,INFINITE)==WAIT_OBJECT_0;}
	bool TryWait( DWORD time=0 ) const {return WaitForSingleObject(_handle,time)==WAIT_OBJECT_0;}
	bool Lock() const {return WaitForSingleObject(_handle,INFINITE)==WAIT_OBJECT_0;}
	bool TryLock() const {return WaitForSingleObject(_handle,0)==WAIT_OBJECT_0;}

	static int WaitForMultiple( SignaledObject *events[], int n, int timeout=-1 );
};

class Event: public SignaledObject
{
	public:
	Event()
	:SignaledObject(CreateEvent(nullptr,FALSE,FALSE,nullptr))
	{
	}
	explicit Event(bool manualReset)
	:SignaledObject(CreateEvent(nullptr,manualReset,FALSE,nullptr))
	{
	}
	void Set(){if( _handle ) SetEvent(_handle);}
	void Reset(){if( _handle ) ResetEvent(_handle);}

};

class Mutex: public SignaledObject
{
	public:
	Mutex()
	:SignaledObject(CreateMutex(nullptr,FALSE,nullptr))
	{}

	void Unlock() const {ReleaseMutex(_handle);}
	bool IsLocked() const
	{
		bool ret=TryLock();
		if( ret ) Unlock();
		return ret;
	}
};

class Semaphore: public SignaledObject
{
	public:
	Semaphore( int init=0, int max=INT_MAX )
	:SignaledObject(CreateSemaphore(nullptr,init,max,nullptr))
	{
	}

	bool Unlock( int count=1 ) const {return ReleaseSemaphore(_handle,count,nullptr)!=FALSE;}
	bool IsLocked() const
	{
		bool ret=TryLock();
		if( ret ) Unlock();
		return ret;
	}
};

class CriticalSection
{
	private:
	mutable CRITICAL_SECTION _handle;

	public:
	CriticalSection(){InitializeCriticalSection(&_handle);}
	~CriticalSection(){DeleteCriticalSection(&_handle);}

	bool Lock() const {EnterCriticalSection(&_handle);return true;}
	void Unlock() const {LeaveCriticalSection(&_handle);}
};

class ScopeLockMutex: public ScopeLock<Mutex>
{
	public:
	ScopeLockMutex( Mutex &lock )
	:ScopeLock<Mutex>(lock)
	{}
	bool TryLock() const {return _lock->TryLock();}
};

typedef ScopeLock<CriticalSection> ScopeLockSection;

#else

// POSIX implementation

class SignaledObject
{
	private:
	SignaledObject ( const SignaledObject &src );
	void operator = ( const SignaledObject &src );

	public:
	SignaledObject ()
	{}
	~SignaledObject()
	{}

	// wait/lock are synonymous
	bool Wait () const
	{ return TRUE; }

	bool TryWait ( DWORD time=0 ) const
	{ return TRUE; }

	bool Lock () const
	{ return TRUE; }

	bool TryLock () const
	{ return TRUE; }

	static int WaitForMultiple ( SignaledObject *events[], int n, int timeout=-1 );
};

class Event: public SignaledObject
{
	public:
	Event ()
	{}
	explicit Event ( bool manualReset )
	{}
	void Set ()
	{}
	void Reset ()
	{}
};

class Mutex: public SignaledObject
{
        protected:
	mutable pthread_mutex_t mutex;

	public:
	Mutex ()
	{
	    mutex = mutexInit;
	}
	void Unlock () const
	{
	    pthread_mutex_unlock(&mutex);
	}
	bool IsLocked () const
	{
	    bool ret = (pthread_mutex_trylock(&mutex) == 0);
	    if ( ret ) Unlock();
	    return ret;
	}
};

class Semaphore: public SignaledObject
{
        protected:
	mutable sem_t sem;

	public:
	Semaphore ( int init=0, int max=INT_MAX )
	{
	    sem_init(&sem,0,(unsigned)init);
	}
	bool Unlock ( int count=1 ) const
	{
	    while ( count-- > 0 )
	        sem_post(&sem);
	    return TRUE;
	}
	bool IsLocked () const
	{
	    int val = 0;
	    if ( sem_getvalue(&sem,&val) != 0 ) val = 0;
	    return( val != 0 );
	}
};

class CriticalSection
{
	private:
	mutable pthread_mutex_t mutex;

	public:
	CriticalSection ()
	{
	    mutex = mutexInit;
	}
	~CriticalSection ()
	{
	    pthread_mutex_destroy(&mutex);
	}
	bool Lock () const
	{
	    pthread_mutex_lock(&mutex);
	    return true;
	}
	void Unlock () const
	{
	    pthread_mutex_unlock(&mutex);
	}
};

class ScopeLockMutex: public ScopeLock<Mutex>
{
	public:
	ScopeLockMutex ( Mutex &lock ) : ScopeLock<Mutex>(lock)
	{}
	bool TryLock() const
	{
	    return _lock->TryLock();
	}
};

typedef ScopeLock<CriticalSection> ScopeLockSection;

#endif
} // namespace Poseidon::Foundation
