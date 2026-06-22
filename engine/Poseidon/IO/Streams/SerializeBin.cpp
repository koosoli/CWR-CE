
#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include <string.h>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Strings/RString.hpp>

namespace Poseidon
{
SerializeBinStream::SerializeBinStream(QIStream* in)
{
    _in = in, _out = nullptr;
    _error = EOK;
}
SerializeBinStream::SerializeBinStream(QOStream* out)
{
    _out = out, _in = nullptr;
    _error = EOK;
}

int SerializeBinStream::TellG()
{
    if (_in)
    {
        return _in->tellg();
    }
    return 0;
}

// Only a value returned by TellG should be passed to SeekG.
void SerializeBinStream::SeekG(int offset)
{
    if (_in)
    {
        _in->seekg(offset, QIOS::beg);
    }
}

bool SerializeBinStream::Version(int ver)
{
    if (_in)
    {
        int fVer = LoadInt();
        return ver == fVer;
    }
    SaveInt(ver);
    return true;
}

#define ENABLE_COMPRESSION 1

#define MIN_COMPRESS_SIZE 1024

void SerializeBinStream::SaveCompressed(const void* data, int size)
{
#if ENABLE_COMPRESSION
    if (size >= MIN_COMPRESS_SIZE)
    {
        SSCompress compress;
        compress.Encode(*_out, (const char*)data, size);
        return;
    }
#endif
    Save(data, size);
}

void SerializeBinStream::LoadCompressed(void* data, int size)
{
#if ENABLE_COMPRESSION
    if (size >= MIN_COMPRESS_SIZE)
    {
        SSCompress compress;
        if (!compress.Decode((char*)data, size, *_in))
        {
            RptF("Error in SerializeBinStream decoding");
            _error = EFileStructure;
        }
        return;
    }
#endif
    Load(data, size);
}

void SerializeBinStream::TransferBinaryCompressed(void* data, int size)
{
    // apply LZW compression - data will be repeated
    // no additional fields required - size is known
    if (_in)
    {
        LoadCompressed(data, size);
    }
    else
    {
        SaveCompressed(data, size);
    }
}

void SerializeBinStream::operator<<(RString& data)
{
    if (IsLoading())
    {
        char buf[4096];
        int n = 0;
        for (;;)
        {
            int c = _in->get();
            if (c <= 0) // NUL terminator or EOF -- get() returns -1 at end of stream,
            {           // which as a char is 0xFF != 0 and would spin the loop forever
                break;
            }
            if (n < static_cast<int>(sizeof(buf)) - 1)
            {
                buf[n++] = static_cast<char>(c);
            }
        }
        buf[n] = 0;
        data = buf;
    }
    else
    {
        // transfer zero terminated string
        int len = strlen(data);
        _out->write(data, len + 1);
    }
}

void SerializeBinStream::operator<<(RStringB& data)
{
    if (IsLoading())
    {
        char buf[4096];
        int n = 0;
        for (;;)
        {
            int c = _in->get();
            if (c <= 0) // NUL terminator or EOF -- get() returns -1 at end of stream,
            {           // which as a char is 0xFF != 0 and would spin the loop forever
                break;
            }
            if (n < static_cast<int>(sizeof(buf)) - 1)
            {
                buf[n++] = static_cast<char>(c);
            }
        }
        buf[n] = 0;
        data = buf;
    }
    else
    {
        // transfer zero terminated string
        int len = strlen(data);
        _out->write(data, len + 1);
    }
}

} // namespace Poseidon
