#ifndef WAVEBUFFER_HPP
#define WAVEBUFFER_HPP

#include <Poseidon/Audio/Streaming/WaveStream.hpp>

#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/IO/Streams/FileCompress.hpp>


namespace Poseidon::Foundation { class FastCAlloc; }
namespace Poseidon
{

// Packed wave data with optional delta compression:
//   deltaPack 0 = uncompressed, 8 = 8-bit delta (2x), 4 = 4-bit delta (4x).
class WaveBuffer
{
	Ref<IFileBuffer> _buffer;
	int _skip;          // offset into the file buffer
	int _size;          // packed-data size
	int _deltaPack;     // compression type (0, 4, or 8)

	// Streaming delta-pack state
	mutable int _lastWriteOffset;
	mutable int _lastValue;

public:
	WAVEFORMATEX _format;

public:
		WaveBuffer(IFileBuffer* buf, const WAVEFORMATEX& format, int start, int size, int deltaPack);

	char* Data() { return (char*)_buffer->GetData() + _skip; }
	const char* Data() const { return (char*)_buffer->GetData() + _skip; }

	int PackedSize() const { return _size; }

	int GetFormat() const { return _deltaPack; }

	void GetWaveFormat(WAVEFORMATEX& format) const { format = _format; }

	bool IsFromBank(QFBank* bank) const;
	
		int UnpackedSize() const
	{
		int size = _size;
		if (_deltaPack == 8) return size * 2;
		if (_deltaPack == 4) return size * 4;
		return size;
	}
	
		void Unpack(void* dest, int destSize) const;
	
		int UnpackPart(void* dest, int offset, int size) const;

private:
	// Fast allocator support
	static Poseidon::Foundation::FastCAlloc& _allocatorF_instance();
};

} // namespace Poseidon

#endif // WAVEBUFFER_HPP
