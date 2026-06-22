#ifndef WAVESTREAMOGG_HPP
#define WAVESTREAMOGG_HPP

#include <Poseidon/Audio/Streaming/WaveStream.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <vorbis/vorbisfile.h>

namespace Poseidon
{
// Decodes OGG Vorbis on the fly via libvorbis, reading through QIFStream
// using vorbisfile callbacks.
class WaveStreamOGG : public WaveStream
{
	mutable QIFStream _file;          // source file stream
	mutable OggVorbis_File _vf;       // vorbis handle (mutable for ov_read/seek in const methods)
	mutable int _bitstream;           // current bitstream index
	mutable int _offset;              // current read offset in bytes

	bool _vfOpen;                     // true once the vorbis file opened
	WAVEFORMATEX _format;             // cached uncompressed format

public:
		WaveStreamOGG(const QIFStream& file);
	
	~WaveStreamOGG();

		bool GetError() const { return !_vfOpen; }

	// WaveStream interface implementation
	int GetUncompressedSize() const override;
	void GetFormat(WAVEFORMATEX& format) const override;
	int GetData(void* data, int offset, int size) const override;
	bool IsFromBank(Poseidon::QFBank* bank) const override;
};

WaveStream* OGGLoadFile(const char* name);

} // namespace Poseidon

#endif // WAVESTREAMOGG_HPP
