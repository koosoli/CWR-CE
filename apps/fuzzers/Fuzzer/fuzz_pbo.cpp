#define _CRT_SECURE_NO_WARNINGS // fopen for the temp .pbo

#include "fuzz_init.hpp"

#include <Poseidon/IO/Streams/QBStream.hpp>

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

// libFuzzer harness for the PBO bank reader (QFBank) -- the outermost container
// every downloaded mod / mission / addon rides inside, so its header bytes (a
// count-driven list of {name, packing method, original size, reserved, timestamp,
// data size} entries) are fully attacker-controlled. QFBank::open is path-based,
// so the input is written to a temp .pbo and opened; ForEach then walks the parsed
// header. A thrown parse error is handled; only a memory-safety fault reaches ASan.
//
// The temp name carries the pid: -jobs/-workers share one CWD, so a fixed name would
// be raced by parallel workers and a torn read would look like a crash.

using namespace Poseidon;

static void PboNoop(const FileInfoO&, const FileBankType*, void*) {}

extern "C" int LLVMFuzzerInitialize(int*, char***)
{
    fuzz::InitEngine();
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    char base[64];
    char pboPath[64];
    snprintf(base, sizeof(base), "fuzz_pbo_tmp_%d", (int)FUZZ_GETPID());
    snprintf(pboPath, sizeof(pboPath), "%s.pbo", base);
    FILE* f = fopen(pboPath, "wb");
    if (!f)
    {
        return 0;
    }
    fwrite(data, 1, size, f);
    fclose(f);

    try
    {
        QFBank bank;
        if (bank.open(RString(base)))
        {
            bank.Lock();
            if (!bank.error())
            {
                bank.ForEach(PboNoop, nullptr);
            }
            bank.Unlock();
        }
    }
    catch (...)
    {
    }
    return 0;
}
