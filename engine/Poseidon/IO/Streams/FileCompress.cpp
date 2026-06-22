
#include <Poseidon/IO/Streams/FileCompress.hpp>
#include <Poseidon/Foundation/Common/FltOpts.hpp>

namespace Poseidon
{
FileBufferUncompressed::FileBufferUncompressed(int outSize, QIStream& in)
{
    _data.Init(outSize); // uncompressed data
    SSCompress ss;
    if (!ss.Decode(_data.Data(), _data.Size(), in))
    {
        _data.Delete();
    }
}

FileBufferSub::FileBufferSub(IFileBuffer* buf, int start, int size) : _whole(buf)
{
    int bufSize = buf->GetSize();
    saturate(start, 0, bufSize);
    saturate(size, 0, bufSize - start);
    _start = start;
    _size = size;
}

} // namespace Poseidon
