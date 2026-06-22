#ifndef WAVESTREAM_HPP
#define WAVESTREAM_HPP

#if defined _WIN32
    #include <Poseidon/Foundation/Common/Win.h>
#endif
#include <Poseidon/Audio/Core/Format/RiffFile.hpp>

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>

namespace Poseidon
{
// Uniform read interface for audio from plain/compressed WAV and OGG sources.
class WaveStream: public RefCount
{
public:
	~WaveStream() override {}
	
		virtual int GetUncompressedSize() const = 0;
	
		virtual void GetFormat(WAVEFORMATEX &format) const = 0;
	
		virtual int GetData(void *data, int offset, int size) const = 0;
	
		virtual bool IsFromBank(Poseidon::QFBank *bank) const = 0;
	
		virtual int GetDataClipped(void *data, int offset, int size) const;
	
		virtual int GetDataLooped(void *data, int offset, int size) const;
};

// Detects the format (WAV/OGG) and creates the matching stream.
WaveStream* SoundLoadFile(const char* name);

WaveStream* SoundLoadMemory(const void* data, size_t size, const char* ext);

WaveStream* WaveLoadFile(const char* name);

WaveStream* WSSLoadFile(const char* name);

} // namespace Poseidon

#endif // WAVESTREAM_HPP
