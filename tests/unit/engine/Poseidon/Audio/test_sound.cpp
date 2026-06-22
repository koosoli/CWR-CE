#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Audio/Core/Format/RiffFile.hpp>
#include <type_traits>
#include <Poseidon/Foundation/Types/Memtype.h>

// Regression: RiffFile.hpp must not introduce a namespace-Poseidon DWORD/WORD
// that shadows the global ::DWORD/::WORD.  It once did (`typedef uint32_t DWORD`
// inside namespace Poseidon, behind an ineffective `#ifndef DWORD` — DWORD is a
// typedef, not a macro).  That made every namespace-Poseidon function using
// DWORD in its signature mangle DWORD as `unsigned int` in any TU including this
// header, while the defining TU used the global ::DWORD (`unsigned long` on
// Windows) — an unresolved-symbol link error for e.g.
// Poseidon::Foundation::GlobalTickCount.  Teeth on Windows (::DWORD ==
// unsigned long != unsigned int); trivially true on Linux (::DWORD == unsigned int).
namespace Poseidon
{
static_assert(std::is_same_v<DWORD, ::DWORD>, "RiffFile.hpp must not shadow global ::DWORD");
static_assert(std::is_same_v<WORD, ::WORD>, "RiffFile.hpp must not shadow global ::WORD");
} // namespace Poseidon

using namespace Poseidon;
TEST_CASE("riffFile: WAVEFORMAT_STRUCT fields", "[Audio]")
{
    WAVEFORMAT_STRUCT wf{};
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 2;
    wf.nSamplesPerSec = 44100;
    wf.nAvgBytesPerSec = 176400;
    wf.nBlockAlign = 4;
    REQUIRE(wf.wFormatTag == 1);
    REQUIRE(wf.nChannels == 2);
    REQUIRE(wf.nSamplesPerSec == 44100);
    REQUIRE(wf.nBlockAlign == 4);
}
