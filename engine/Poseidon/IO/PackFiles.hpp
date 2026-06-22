#pragma once

#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/IO/Streams/FileInfo.h>


namespace Poseidon
{
class SOFStream;

// Zero(x) is a Base/Common/global.hpp macro (memset((void*)&(x), …)) that
// collides with FileInfoExt::Zero() below. Undef unconditionally — needed on
// Windows once master-server includes pull global.hpp through this header.
#undef Zero

class IFilebankEncryption;

struct FileInfoExt
{
	RString name;
	int compressedMagic;
	int uncompressedSize;
	int32_t startOffset;
	int32_t time;
	int32_t length;
	int priority;

	void Zero(){memset(this,0,sizeof(*this));}
	const char *GetKey() const {return name;}

	// >= 0 gives order of processing
	// small numbers should be included first
	// < 0 means file should be excluded
	FileInfoExt(){priority=0;}
};




extern const char *DefFileBankNoCompress[];
extern const char *DefFileBankEncrypt[];

struct QFProperty;

//! utility to create file banks (pbo files)
class FileBankManager
{
	AutoArray<FileInfoExt> files;
	int newestFile;
	int size;

	public:
	FileBankManager();
	~FileBankManager();
	//! create a pbo file bank
	LSError Create
	(
		const char *tgt, const char *src,
		bool compress=false, bool optimize=true,
		const char *logFile=nullptr,
		const char **doNotCompress=DefFileBankNoCompress,
		const QFProperty *properties=nullptr, int nProperties=0
	);
	// optimize creates pbo file with short header
	// set false on if you need to create old pbf
	void Create
	(
		QOStream &out, const char *src, 
		bool compress=false, bool optimize=true,
		const char *logFile=nullptr,
		const char **doNotCompress=DefFileBankNoCompress,
		const QFProperty *properties=nullptr, int nProperties=0
	);
	//! create encrypted pbo file
	void Create
	(
		QOStream &out, const char *src, 
		IFilebankEncryption *encrypt,
		const QFProperty *properties, int nProperties,
		const char **encryptExts=DefFileBankEncrypt
	);
	void ParseMasks(const char *logFile);
	void SortAndRemove(bool remove);

	// create bank in memory
	void ScanDir( RString dir, RString rel);

	void SaveHeadersOpt( QOStream &out);
	void SaveHeadersOpt( SOFStream &out);

	void SaveProperties( QOStream &out, const QFProperty *prop, int nProp );
	void SaveProperties( SOFStream &out, const QFProperty *prop, int nProp );
};

} // namespace Poseidon

using Poseidon::FileBankManager;
