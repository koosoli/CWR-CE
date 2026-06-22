#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Audio/Streaming/WaveStreamPlain.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>

namespace Poseidon
{

WaveStreamPlain::WaveStreamPlain(const WaveBuffer& buffer) : _data(buffer) {}

void WaveStreamPlain::GetFormat(WAVEFORMATEX& format) const
{
    _data.GetWaveFormat(format);
}

int WaveStreamPlain::GetUncompressedSize() const
{
    return _data.UnpackedSize();
}

int WaveStreamPlain::GetData(void* data, int offset, int size) const
{
    // Assume _data is uncompressed (or internally decompressed by WaveBuffer)
    // Therefore we can unpack from any position
    int rest = _data.UnpackedSize() - offset;
    if (rest > 0)
    {
        int unpack = size;
        saturateMin(unpack, rest);
        int unpacked = _data.UnpackPart(data, offset, unpack);

        return unpacked;
    }
    return 0;
}

bool WaveStreamPlain::IsFromBank(Poseidon::QFBank* bank) const
{
    return _data.IsFromBank(bank);
}

} // namespace Poseidon
