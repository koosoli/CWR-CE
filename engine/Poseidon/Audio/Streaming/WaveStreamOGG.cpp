#include <Poseidon/Foundation/Common/Win.h>
#include <Poseidon/Core/Application.hpp>

#include <Poseidon/Audio/Streaming/WaveStreamOGG.hpp>
#include <Poseidon/IO/FileServer.hpp>
#include <vorbis/vorbisfile.h>
#include <ogg/config_types.h>
#include <stdio.h>
#include <vorbis/codec.h>
#include <Poseidon/Foundation/Framework/Log.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>
#include <Poseidon/Foundation/Types/Pointers.hpp>

namespace Poseidon
{

// Vorbis callback interface to QIFStream

static size_t readQIFStream(void* buffer, size_t size, size_t count, void* stream)
{
    QIFStream* str = static_cast<QIFStream*>(stream);
    int rd = static_cast<int>(size * count);
    int rest = str->rest();
    if (rd > rest)
    {
        rd = rest;
    }
    str->read(buffer, rd);
    return rd;
}

static int seekQIFStream(void* stream, ogg_int64_t pos, int origin)
{
    QIFStream* str = static_cast<QIFStream*>(stream);
    switch (origin)
    {
        case SEEK_CUR:
            str->seekg(static_cast<int>(pos), QIOS::cur);
            break;
        case SEEK_END:
            str->seekg(static_cast<int>(pos), QIOS::end);
            break;
        case SEEK_SET:
            str->seekg(static_cast<int>(pos), QIOS::beg);
            break;
    }
    // No error
    return 0;
}

static int closeQIFStream(void* stream)
{
    // No close required
    return 0;
}

static long tellQIFStream(void* stream)
{
    QIFStream* str = static_cast<QIFStream*>(stream);
    return str->tellg();
}

WaveStreamOGG::WaveStreamOGG(const QIFStream& file)
{
    _file = file;
    ov_callbacks callbacks;
    callbacks.read_func = readQIFStream;
    callbacks.seek_func = seekQIFStream;
    callbacks.close_func = closeQIFStream;
    callbacks.tell_func = tellQIFStream;

    int ok = ov_open_callbacks(&_file, &_vf, nullptr, 0, callbacks);

    _vfOpen = ok >= 0;
    _bitstream = 0;
    _offset = 0;

    // Get uncompressed format
    if (_vfOpen)
    {
        vorbis_info* info = ov_info(&_vf, -1);

        _format.cbSize = 0;
        _format.wFormatTag = WAVE_FORMAT_PCM;
        _format.wBitsPerSample = 16;
        _format.nChannels = static_cast<WORD>(info->channels);
        _format.nBlockAlign = static_cast<WORD>(info->channels * 2);
        _format.nSamplesPerSec = info->rate;
        _format.nAvgBytesPerSec = info->rate * 2 * info->channels;
    }
}

WaveStreamOGG::~WaveStreamOGG()
{
    if (_vfOpen)
    {
        _vfOpen = false;
        ov_clear(&_vf);
    }
}

int WaveStreamOGG::GetUncompressedSize() const
{
    if (!_vfOpen)
    {
        return 0;
    }
    int total = static_cast<int>(ov_pcm_total(&_vf, -1));
    // Convert PCM samples to bytes
    return total * _format.nBlockAlign;
}

void WaveStreamOGG::GetFormat(WAVEFORMATEX& format) const
{
    // Get uncompressed format
    if (!_vfOpen)
    {
        format.cbSize = 0;
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.wBitsPerSample = 16;
        format.nChannels = 1;
        format.nBlockAlign = 2;
        format.nSamplesPerSec = 22000;
        format.nAvgBytesPerSec = 44000;
        return;
    }
    vorbis_info* info = ov_info(&_vf, -1);

    format.cbSize = 0;
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.wBitsPerSample = 16;
    format.nChannels = static_cast<WORD>(info->channels);
    format.nBlockAlign = static_cast<WORD>(info->channels * 2);
    format.nSamplesPerSec = info->rate;
    format.nAvgBytesPerSec = info->rate * 2 * info->channels;
}

int WaveStreamOGG::GetData(void* data, int offset, int size) const
{
    if (!_vfOpen)
    {
        return 0;
    }
    if (_offset != offset)
    {
        int pcm = offset / (_format.nChannels * 2);
        // We need to seek in the source stream.
        int err = ov_pcm_seek(&_vf, pcm);
        if (err != 0)
        {
            // Ignore error, but report it
            LOG_DEBUG(Audio, "Error {} while seeking", err);
        }
        _offset = offset;
    }

    int dataDone = 0;
    while (size > 0)
    {
        int obs = _bitstream;
        int rd = ov_read(&_vf, static_cast<char*>(data), size, 0, 2, 1, &_bitstream);
        if (_bitstream != obs)
        {
            LOG_DEBUG(Audio, "Stream {}", _bitstream);
        }
        if (rd <= 0)
        {
            LOG_DEBUG(Audio, "Stream read error");
            break;
        }

        size -= rd;
        data = static_cast<char*>(data) + rd;
        dataDone += rd;
        _offset += rd;
    }
    return dataDone;
}

bool WaveStreamOGG::IsFromBank(Poseidon::QFBank* bank) const
{
    return _file.GetBuffer()->IsFromBank(bank);
}

WaveStream* OGGLoadFile(const char* name)
{
    QIFStream in;
    GFileServer->Open(in, name);
    if (in.fail() || in.eof())
    {
        return nullptr;
    }

    if (in.rest() <= 0)
    {
        return nullptr;
    }

    WaveStreamOGG* ret = new WaveStreamOGG(in);
    if (ret->GetError())
    {
        delete ret;
        return nullptr;
    }
    return ret;
}

} // namespace Poseidon
