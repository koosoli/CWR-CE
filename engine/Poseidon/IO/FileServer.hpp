#pragma once

#include <Poseidon/IO/Streams/QStream.hpp>

// file buffering class, suitable for background (multithreaded) usage


namespace Poseidon
{
class QFBank;

class FileServer: public RefCount
{

	public:
	// abstract class - single/multi threaded file loading
	// say in advance we will need the file - insert it into the queue
	virtual void Request( const char *name, float time, int from=0, int to=INT_MAX)= 0;
	virtual void CancelRequest( const char *name, int from=0, int to=INT_MAX )= 0;
	// say we need the file NOW - load it
	virtual void Open( QIFStream &stream, const char *name ) = 0;

	virtual void Start() = 0; // start/stop background activity
	virtual void Stop() = 0;

	virtual void FlushBank(QFBank *bank) = 0;
    
};

extern Ref<FileServer> GFileServer;

} // namespace Poseidon

using Poseidon::GFileServer;
extern bool GEnableCaching;
