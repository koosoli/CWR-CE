#define _CRT_SECURE_NO_WARNINGS // fopen for the temp .lip

#include "fuzz_init.hpp"

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/World/Entities/Infantry/Head.hpp>
#include <Poseidon/Audio/Dummy/WaveDummy.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#ifdef _WIN32
#include <process.h>
#define FUZZ_GETPID _getpid
#else
#include <unistd.h>
#define FUZZ_GETPID getpid
#endif

// libFuzzer harness for the .lip lip-sync reader (ManLipInfo).
//
// A .lip rides paired with a voice .ogg/.wav inside untrusted mission / addon
// content. ManLipInfo::AttachWave derives "<wave>.lip" from the wave name and parses
// it line-by-line: readLine into a fixed buffer, then sscanf "%f, %d" (time, phase)
// or "frame = %f". The input is written to a temp .lip; a dummy wave named for the
// sibling .ogg drives the real attach + parse path (so readLine and the line loop —
// shared infra — are exercised exactly as production runs them). A thrown error is
// handled; only a memory-safety fault reaches ASan.
//
// The temp name carries the pid: -jobs/-workers share one CWD, so a fixed name would
// be raced by parallel workers and a torn read would look like a crash.

using namespace Poseidon;

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    char lipPath[64];
    char wavePath[64];
    snprintf(lipPath, sizeof(lipPath), "fuzz_lip_tmp_%d.lip", (int)FUZZ_GETPID());
    snprintf(wavePath, sizeof(wavePath), "fuzz_lip_tmp_%d.ogg", (int)FUZZ_GETPID());

    FILE* fp = fopen(lipPath, "wb");
    if (!fp)
    {
        return 0;
    }
    fwrite(data, 1, size, fp);
    fclose(fp);

    try
    {
        Ref<WaveDummy> wave = new WaveDummy(RString(wavePath));
        ManLipInfo lip;
        lip.AttachWave(wave, 1.0f);
    }
    catch (...)
    {
    }
    return 0;
}
