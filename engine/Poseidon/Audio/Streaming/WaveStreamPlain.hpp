#ifndef WAVESTREAMPLAIN_HPP
#define WAVESTREAMPLAIN_HPP

#include <Poseidon/Audio/Streaming/WaveStream.hpp>
#include <Poseidon/Audio/Core/Format/WaveBuffer.hpp>

namespace Poseidon
{
// Thinnest WaveStream — wraps a WaveBuffer and delegates every read to it
// (UnpackPart provides uncompressed output from any internal delta packing).
class WaveStreamPlain : public WaveStream
{
	WaveBuffer _data;  // source audio buffer

public:
		WaveStreamPlain(const WaveBuffer& buffer);

	// WaveStream interface implementation
	int GetUncompressedSize() const override;
	void GetFormat(WAVEFORMATEX& format) const override;
	int GetData(void* data, int offset, int size) const override;
	bool IsFromBank(Poseidon::QFBank* bank) const override;
};

} // namespace Poseidon

#endif // WAVESTREAMPLAIN_HPP
