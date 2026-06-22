#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <cstdint>
#include <catch2/catch_message.hpp>

// Security PoCs - decoder memory-safety finds from the libFuzzer campaign.
// Companion to test_server_network_exploits_poc.cpp. These are the DISTINCT
// memory-safety bugs the coverage-guided harness (apps/Fuzzer/fuzz_decode_msg)
// surfaced in NetworkComponent::DecodeMessage with saved reproducers, beyond the
// unbounded array count already covered there (C4). The vulnerable code sits
// below the CRC in OnServerUserMessage and can't be unit-instantiated without
// the live UDP stack, so each PoC transcribes the engine's current logic
// VERBATIM (with the cited file:line and the ASan signature the fuzzer reported)
// and pairs it with the proposed-fix predicate. "_current" assertions are green
// today = they document the live hole; "_fixed" assertions are the spec a fix
// must satisfy = the regression teeth. Fixes are deferred behind the dispatch
// redesign; these lock the behaviour so a fix can be proven against them.
//
//   D1 NetworkServerIntegrity.cpp:1313  GetFormat - negative wire type indexes
//                                       the global GMsgFormats[] table OOB
//                                       (fuzzer: global-buffer-overflow @ DecodeMessage)
//   D2 NetworkMsgContext.cpp:1126-1130  Get(RString,NCTStringGeneric) - wire id
//                                       indexes the string table with no upper bound
//                                       (fuzzer: heap-buffer-overflow @ :1128, dominant)
//   D3 NetworkMessages.hpp:104-109      NetworkMessageRaw::Read - the bounds check
//                                       `int minSize = _pos + size` overflows int
//                                       (fuzzer: heap-buffer-overflow @ :106)

namespace
{
// ---- D1: message-type table index ------------------------------------------
// Engine (NetworkServer::GetFormat, NetworkServerIntegrity.cpp:1313):
//   PoseidonAssert(type >= 0);          // compiled out under NDEBUG (RC3)
//   if (type >= NMTN) return nullptr;
//   return GMsgFormats[type];
// Under NDEBUG the only surviving runtime bound is the upper one, so a negative
// type (NCTSmallUnsigned decodes a >INT_MAX varint into a negative int) reaches
// GMsgFormats[type] out of bounds. The harness reproduces this verbatim.
bool FormatIndexInRange_current(int type, int nmtn)
{
    return type < nmtn; // verbatim surviving bound; the `type >= 0` assert is gone
}
bool FormatIndexInRange_fixed(int type, int nmtn)
{
    return type >= 0 && type < nmtn;
}

// ---- D2: string-table index ------------------------------------------------
// Engine (Get(RString&, NCTStringGeneric), NetworkMsgContext.cpp:1126-1130):
//   if (id > 0) { value = GetNetIdStrings().GetString(id - 1); return true; }
// `id` is an attacker varint checked only for `> 0`; it is never bounded against
// the string-table size before GetString(id - 1) indexes it.
bool StringIdAccepted_current(int id, int /*tableSize*/)
{
    return id > 0; // verbatim: only the lower bound is checked
}
bool StringIdAccepted_fixed(int id, int tableSize)
{
    return id > 0 && id <= tableSize; // must address a real table entry
}

// ---- D3: Read bounds-check integer overflow --------------------------------
// Engine (NetworkMessageRaw::Read, NetworkMessages.hpp:104-109):
//   int minSize = _pos + size;
//   if (_externalBufferSize < minSize) return false;
//   memcpy(buffer, _externalBuffer + _pos, size);
// `_pos + size` is 32-bit signed; a large `size` overflows it to a negative
// minSize, so the `< minSize` guard passes and the memcpy reads out of bounds.
bool ReadInBounds_current(int pos, int size, int bufSize)
{
    int minSize = pos + size;  // verbatim 32-bit arithmetic — can overflow
    return bufSize >= minSize; // guard returns "in bounds" on a wrapped minSize
}
bool ReadInBounds_fixed(int pos, int size, int bufSize)
{
    if (pos < 0 || size < 0 || bufSize < 0)
        return false;
    return static_cast<int64_t>(pos) + static_cast<int64_t>(size) <= static_cast<int64_t>(bufSize);
}
} // namespace

TEST_CASE("PoC D1: negative wire type indexes the GMsgFormats table out of bounds",
          "[security][poc][network][oob][fuzz]")
{
    const int kNMTN = 64; // models the number of message-format slots in GMsgFormats[]

    // Ground the claim in a real container the size of the format table.
    AutoArray<void*> formatTable;
    formatTable.Resize(kNMTN);
    REQUIRE(formatTable.Size() == kNMTN);

    WARN("PoC D1: GetFormat returns GMsgFormats[type] for a negative type (NDEBUG strips the assert)");
    // EXPLOIT: a negative decoded type clears the only surviving bound.
    REQUIRE(FormatIndexInRange_current(-1, kNMTN));        // negative index accepted
    REQUIRE(FormatIndexInRange_current(INT32_MIN, kNMTN)); // wrapped varint accepted
    const bool negTypeInBounds = (-1 >= 0) && (-1 < formatTable.Size());
    REQUIRE_FALSE(negTypeInBounds); // ...yet -1 is outside [0, Size())

    // Proposed fix: index must be a real slot; legitimate types still pass.
    REQUIRE_FALSE(FormatIndexInRange_fixed(-1, kNMTN));
    REQUIRE_FALSE(FormatIndexInRange_fixed(INT32_MIN, kNMTN));
    REQUIRE(FormatIndexInRange_fixed(0, kNMTN));
    REQUIRE(FormatIndexInRange_fixed(kNMTN - 1, kNMTN));
    REQUIRE_FALSE(FormatIndexInRange_fixed(kNMTN, kNMTN)); // upper bound still enforced
}

TEST_CASE("PoC D2: wire string id indexes the net-id string table with no upper bound",
          "[security][poc][network][oob][fuzz]")
{
    const int kTableSize = 8; // a small string table; the attacker id dwarfs it
    const int huge = 0x7FFFFFFF;

    // Ground the claim: GetString(id - 1) on a real array of this size.
    AutoArray<int> stringTable;
    stringTable.Resize(kTableSize);
    REQUIRE(stringTable.Size() == kTableSize);

    WARN("PoC D2: Get(RString) does GetString(id-1) with id checked only for >0");
    // EXPLOIT: any positive id passes; id-1 then indexes far past the table.
    REQUIRE(StringIdAccepted_current(huge, kTableSize));
    const int idx = huge - 1; // the actual index used
    const bool idInBounds = (idx >= 0 && idx < stringTable.Size());
    REQUIRE_FALSE(idInBounds); // outside [0, Size())

    // Proposed fix: id must address a real table entry; valid ids still pass.
    REQUIRE_FALSE(StringIdAccepted_fixed(huge, kTableSize));
    REQUIRE_FALSE(StringIdAccepted_fixed(0, kTableSize));    // 0 is the "inline string" path, not an index
    REQUIRE(StringIdAccepted_fixed(1, kTableSize));          // first entry
    REQUIRE(StringIdAccepted_fixed(kTableSize, kTableSize)); // last entry
    REQUIRE_FALSE(StringIdAccepted_fixed(kTableSize + 1, kTableSize));
}

TEST_CASE("PoC D3: Read's _pos+size bounds check overflows int and admits an OOB read",
          "[security][poc][network][oob][fuzz]")
{
    const int bufSize = 16; // a tiny external buffer

    // Two near-INT_MAX values whose sum wraps negative.
    const int pos = 0x40000000;  // 2^30
    const int size = 0x40000000; // 2^30; pos + size = 0x80000000 = INT_MIN

    WARN("PoC D3: int minSize = _pos + size overflows; the < check passes and memcpy reads OOB");
    // EXPLOIT: the wrapped (negative) minSize makes the guard report "in bounds".
    REQUIRE(ReadInBounds_current(pos, size, bufSize));
    // Sanity: the read truly runs off the end (64-bit truth).
    REQUIRE(static_cast<int64_t>(pos) + static_cast<int64_t>(size) > static_cast<int64_t>(bufSize));

    // Proposed fix: 64-bit math + non-negative guards reject it; valid reads pass.
    REQUIRE_FALSE(ReadInBounds_fixed(pos, size, bufSize));
    REQUIRE(ReadInBounds_fixed(0, bufSize, bufSize));  // read exactly to the end
    REQUIRE(ReadInBounds_fixed(8, 8, bufSize));        // read the second half
    REQUIRE_FALSE(ReadInBounds_fixed(8, 9, bufSize));  // one past the end
    REQUIRE_FALSE(ReadInBounds_fixed(-1, 4, bufSize)); // negative position
}
