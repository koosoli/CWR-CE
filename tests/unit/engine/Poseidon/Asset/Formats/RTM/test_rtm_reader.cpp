#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Asset/Formats/RTM/RTMReader.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Regression for the RTM bone-name stack-buffer-overflow the libFuzzer harness
// (apps/Fuzzer/fuzz_rtm) found: readRTMHeader read a fixed BONE_NAME_LEN-byte
// name into a same-sized buffer, then scanned it for a NUL (tolower loop + the
// std::string assignment). A name with no NUL in those bytes ran the scan off
// the stack. The fix gives the buffer one extra guaranteed-NUL byte.

namespace
{
void Put(std::vector<uint8_t>& b, const void* p, size_t n)
{
    const uint8_t* s = static_cast<const uint8_t*>(p);
    b.insert(b.end(), s, s + n);
}
} // namespace

TEST_CASE("RTM reader bounds an un-terminated bone name to its field width", "[rtm][asset][oob][fuzz]")
{
    using namespace Poseidon::Asset::Formats;

    std::vector<uint8_t> rtm;
    Put(rtm, "RTM_0101", 8); // magic (V101)
    float step[3] = {0.0f, 0.0f, 0.0f};
    Put(rtm, step, sizeof(step)); // stepX/Y/Z
    int32_t nAnim = 0;
    Put(rtm, &nAnim, sizeof(nAnim)); // 0 phases
    int32_t nSel = 1;
    Put(rtm, &nSel, sizeof(nSel)); // 1 bone

    // 32 bytes, all non-zero: no NUL terminator anywhere in the name field.
    uint8_t name[BONE_NAME_LEN];
    std::memset(name, 'A', sizeof(name));
    Put(rtm, name, sizeof(name));

    RTMAnimation anim;
    const bool ok = readRTMFromMemory(rtm.data(), rtm.size(), anim);

    // Pre-fix: the scan overran the 32-byte stack buffer (ASan stack-buffer-overflow
    // under the fuzz build; a too-long / garbage name otherwise). Post-fix the name
    // is exactly the field width, lowercased.
    REQUIRE(ok);
    REQUIRE(anim.boneCount() == 1);
    REQUIRE(anim.boneNames[0].size() == static_cast<size_t>(BONE_NAME_LEN));
    REQUIRE(anim.boneNames[0] == std::string(BONE_NAME_LEN, 'a'));
}
