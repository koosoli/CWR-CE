#include <catch2/catch_test_macros.hpp>

#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Foundation/Containers/Array.hpp>
#include <Poseidon/Foundation/Types/Pointers.hpp>

#include <cstdint>
#include <cstring>

using namespace Poseidon;

// Loading a corrupt/old savegame must fail gracefully, never crash. A savegame is a
// SerializeBinStream; each array begins with a 4-byte element count. A malformed count (the
// classic corrupt/truncated-save vector) must be rejected with EFileStructure BEFORE it drives a
// huge/negative Realloc or reads past the buffer — locking the SerializeBinStream hardening
// (commit fcc6e9892), which had no unit test.
//
// Broken-state delta: without the `size < 0 || size > GetRest()` guard, the huge-count case
// Reallocs ~8 GB (or the int-overflow path under-allocates then writes OOB), and the negative
// count is undefined — i.e. exactly the "load old save -> crash" class.

namespace
{
SerializeBinStream::ErrorCode LoadCount(const void* bytes, int len, AutoArray<int>& out)
{
    QIStream in(bytes, len);
    SerializeBinStream ar(&in);
    ar.TransferBasicArray(out);
    return ar.GetError();
}

// Minimal ref-counted element for the TransferRefArray guard test.
class RefTestObj : public RefCount
{
  public:
    void SerializeBin(SerializeBinStream&) {}
};
} // namespace

TEST_CASE("SerializeBinStream rejects a huge array count", "[io][serialize][saves]")
{
    // count = INT32_MAX, but no element bytes follow.
    int32_t huge = 0x7FFFFFFF;
    char buf[4];
    memcpy(buf, &huge, sizeof(huge));

    AutoArray<int> data;
    REQUIRE(LoadCount(buf, sizeof(buf), data) == SerializeBinStream::EFileStructure);
    REQUIRE(data.Size() == 0); // no multi-GB allocation
}

TEST_CASE("SerializeBinStream rejects a negative array count", "[io][serialize][saves]")
{
    int32_t neg = -1;
    char buf[4];
    memcpy(buf, &neg, sizeof(neg));

    AutoArray<int> data;
    REQUIRE(LoadCount(buf, sizeof(buf), data) == SerializeBinStream::EFileStructure);
    REQUIRE(data.Size() == 0);
}

TEST_CASE("SerializeBinStream round-trips a well-formed array", "[io][serialize][saves]")
{
    // count = 3, then three ints — a valid stream must load cleanly.
    int32_t n = 3;
    int32_t vals[3] = {10, 20, 30};
    char buf[4 + sizeof(vals)];
    memcpy(buf, &n, 4);
    memcpy(buf + 4, vals, sizeof(vals));

    AutoArray<int> data;
    REQUIRE(LoadCount(buf, sizeof(buf), data) == SerializeBinStream::EOK);
    REQUIRE(data.Size() == 3);
    REQUIRE(data[0] == 10);
    REQUIRE(data[1] == 20);
    REQUIRE(data[2] == 30);
}

// The two sibling array transfers that the original count-hardening missed. Reached from
// untrusted P3D/RTM model SerializeBin (ShapeFile / Animation). Found by the static sweep.

TEST_CASE("SerializeBinStream::TransferBinaryArray rejects a negative count", "[io][serialize][saves]")
{
    // Broken-state delta: without the size<0 guard, Realloc(-1) calls MemAlloc with a
    // ~2^64 size_t (Resize clamps negatives, Realloc does not) -> allocation-size-too-big.
    int32_t neg = -1;
    char buf[4];
    memcpy(buf, &neg, sizeof(neg));

    QIStream in(buf, sizeof(buf));
    SerializeBinStream ar(&in);
    AutoArray<int> data;
    ar.TransferBinaryArray(data);
    REQUIRE(ar.GetError() == SerializeBinStream::EFileStructure);
    REQUIRE(data.Size() == 0); // no huge Realloc
}

TEST_CASE("SerializeBinStream::TransferRefArray rejects an unbacked count", "[io][serialize][saves]")
{
    // Broken-state delta: without the size<0 || size>GetRest() guard, a huge count drives
    // Realloc(size) + a size-iteration `new Type` loop (OOM) before any element is read.
    int32_t huge = 0x7FFFFFFF;
    char buf[4];
    memcpy(buf, &huge, sizeof(huge));

    QIStream in(buf, sizeof(buf));
    SerializeBinStream ar(&in);
    RefArray<RefTestObj> data;
    ar.TransferRefArray(data);
    REQUIRE(ar.GetError() == SerializeBinStream::EFileStructure);
    REQUIRE(data.Size() == 0); // no huge Realloc / new-loop
}
