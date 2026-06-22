#include <Poseidon/Foundation/Common/Win.h>

#include <Poseidon/Audio/Streaming/WaveLoaders.hpp>
#include <Poseidon/Audio/Streaming/WaveStreamPlain.hpp>
#include <Poseidon/Audio/Core/Format/WaveBuffer.hpp>
#include <Poseidon/IO/FileServer.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

namespace Poseidon
{

// WSS file format support

#ifdef _MSC_VER
const int WSSMagic = '0SSW';
#else
const int WSSMagic = StrToInt("WSS0");
#endif

struct WSSHeader
{
    int magic;
    char deltaPack;
    char resvd[3];
};

WaveStream* WSSLoadFile(const char* name)
{
    QIFStream in;
    GFileServer->Open(in, name);
    if (in.fail() || in.eof())
    {
        return nullptr;
    }
    WSSHeader wHeader;
    WAVEFORMATEX header;
    in.read(reinterpret_cast<char*>(&wHeader), sizeof(wHeader));
    if (wHeader.magic != WSSMagic)
    {
        in.seekg(-static_cast<int>(sizeof(wHeader)), QIOS::cur); // Intentional: seek backward
        wHeader.deltaPack = 0;
    }
    int deltaPack = wHeader.deltaPack;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (in.fail() || in.eof())
    {
        return nullptr;
    }
    header.cbSize = 0;
    IFileBuffer* fBuffer = in.GetBuffer();

    if (in.fail() || in.eof())
    {
        return nullptr;
    }
    if (in.rest() <= 0)
    {
        return nullptr;
    }
    // Return part of file buffer

    WaveBuffer buffer(fBuffer, header, in.tellg(), in.rest(), deltaPack);
    return new WaveStreamPlain(buffer);
}

} // namespace Poseidon
