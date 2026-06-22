#pragma once
#include <Poseidon/Foundation/Types/Memtype.h>

namespace Poseidon
{
/* general waveform format structure (information common to all formats) */
#pragma pack(push, 1)
struct WAVEFORMAT_STRUCT {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} ;
#pragma pack(pop)
typedef WAVEFORMAT_STRUCT WAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM     1
#endif

/* specific waveform format structure for PCM data — packed for binary I/O */
#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
#pragma pack(push, 1)
typedef struct {
    WORD        wFormatTag;
    WORD        nChannels;
    DWORD       nSamplesPerSec;
    DWORD       nAvgBytesPerSec;
    WORD        nBlockAlign;
    WORD        wBitsPerSample;
    WORD        cbSize;
} WAVEFORMATEX;
#pragma pack(pop)
#endif
} // namespace Poseidon
