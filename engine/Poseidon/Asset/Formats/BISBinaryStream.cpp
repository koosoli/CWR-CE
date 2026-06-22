#include <Poseidon/Asset/Formats/BISBinaryStream.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>

namespace Poseidon::Asset::Formats
{

bool BinaryReader::decompressLZSS(void* dst, size_t size)
{
    SSCompress decompressor;
    return decompressor.Decode(static_cast<char*>(dst), static_cast<long>(size), stream_);
}

} // namespace Poseidon::Asset::Formats
