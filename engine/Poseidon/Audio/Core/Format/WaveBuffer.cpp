#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Audio/Core/Format/WaveBuffer.hpp>
#include <Poseidon/Audio/Core/Format/DeltaPack.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <string.h>
#include <Poseidon/Foundation/Common/FltOpts.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Memory/FastAlloc.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{

DEFINE_FAST_ALLOCATOR(WaveBuffer)

WaveBuffer::WaveBuffer(IFileBuffer* buf, const WAVEFORMATEX& format, int start, int size, int deltaPack)
    : _buffer(buf), _format(format), _skip(start), _size(size), _deltaPack(deltaPack), _lastWriteOffset(0),
      _lastValue(0)
{
    // The packed size can come from a declared RIFF/WSS chunk length that exceeds
    // the bytes actually present (a truncated or lying header); clamp the start and
    // size to the buffer so UnpackPart/Unpack cannot read past it. A valid file has
    // start + size <= buffer size, so this leaves it unchanged.
    const int avail = _buffer ? _buffer->GetSize() : 0;
    if (_skip < 0)
        _skip = 0;
    if (_skip > avail)
        _skip = avail;
    if (_size < 0)
        _size = 0;
    if (_size > avail - _skip)
        _size = avail - _skip;
}

bool WaveBuffer::IsFromBank(QFBank* bank) const
{
    return _buffer->IsFromBank(bank);
}

int WaveBuffer::UnpackPart(void* dest, int offset, int size) const
{
    int maxSize = UnpackedSize() - offset;
    saturateMin(size, maxSize);

    if (_deltaPack == 0)
    {
        // Uncompressed - direct copy
        memcpy(dest, Data() + offset, size);
    }
    else if (_deltaPack == 4)
    {
        // 4-bit delta packing (4x compression)
        // Check if offset changed - need to restart delta unpacker
        if (offset != _lastWriteOffset)
        {
            _lastValue = UnpackD4.Skip(Data(), offset, 0);
        }

        _lastValue = UnpackD4.Unpack((short*)dest, Data() + offset / 4, size, _lastValue);
        _lastWriteOffset = offset + size;
    }
    else if (_deltaPack == 8)
    {
        // 8-bit delta packing (2x compression)
        if (offset != _lastWriteOffset)
        {
            _lastValue = UnpackD8.Skip(Data(), offset, 0);
        }

        _lastValue = UnpackD8.Unpack((short*)dest, Data() + offset / 2, size, _lastValue);
        _lastWriteOffset = offset + size;
    }
    else
    {
        memset(dest, 0, size);
        Fail("Invalid deltaPack value");
    }

    return size;
}

void WaveBuffer::Unpack(void* dest, int destSize) const
{
    if (_deltaPack == 0)
    {
        // Uncompressed
        PoseidonAssert(destSize == UnpackedSize());
        memcpy(dest, Data(), destSize);
    }
    else if (_deltaPack == 4)
    {
        // 4-bit delta packing
        PoseidonAssert(destSize == UnpackedSize());
        UnpackD4.Unpack((short*)dest, Data(), UnpackedSize(), 0);
    }
    else if (_deltaPack == 8)
    {
        // 8-bit delta packing
        PoseidonAssert(destSize == UnpackedSize());
        UnpackD8.Unpack((short*)dest, Data(), UnpackedSize(), 0);
    }
    else
    {
        Fail("Invalid deltaPack value");
    }
}

} // namespace Poseidon
