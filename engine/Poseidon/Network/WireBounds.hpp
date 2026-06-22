#pragma once

#include <cstdint>

// Decode-check primitives shared by the message handlers.
//
// Centralizes the validate-every-wire-read discipline (check every wire-read
// count/length against a sane cap, reject negatives, refuse a count*size that
// overflows int before any Resize/memcpy) into reusable, branch-free predicates,
// so the check is identical at every call site instead of re-derived per handler.

namespace Poseidon
{

namespace WireBounds
{

// A wire-supplied element count is acceptable: non-negative and within a
// per-field sane cap. Use before Resize(n) / for-loops driven by a wire count.
inline bool ValidCount(int count, int cap)
{
    return count >= 0 && count <= cap;
}

// A wire-supplied byte length is acceptable. Same contract as ValidCount;
// named separately so call sites read in their own units.
inline bool ValidLength(int length, int cap)
{
    return length >= 0 && length <= cap;
}

// count * elemSize fits in a non-negative int. Guards the N-SEC-05 pattern where
// an attacker count makes the allocation size wrap negative/small. Both inputs
// must already be non-negative (pair with ValidCount); elemSize is a sizeof.
inline bool MulFitsInt(int count, int elemSize)
{
    if (count < 0 || elemSize <= 0)
    {
        return false;
    }
    return static_cast<int64_t>(count) * static_cast<int64_t>(elemSize) <= static_cast<int64_t>(INT32_MAX);
}

// A wire offset+span stays within a buffer: offset and span non-negative and
// offset+span <= size. Guards memcpy(base+offset, …, span) against OOB writes.
inline bool RangeInBounds(int offset, int span, int size)
{
    if (offset < 0 || span < 0 || size < 0)
    {
        return false;
    }
    return static_cast<int64_t>(offset) + static_cast<int64_t>(span) <= static_cast<int64_t>(size);
}

// Number of fixed-size trailing array elements that actually fit in a packet of
// `payloadBytes` after a `headerBytes` header. Bounds a flexible-array-member loop
// to the elements really present, rather than to attacker-supplied bounds (N-SEC-13).
inline int TrailingElementCount(int payloadBytes, int headerBytes, int elemBytes)
{
    if (payloadBytes < headerBytes || elemBytes <= 0)
    {
        return 0;
    }
    return (payloadBytes - headerBytes) / elemBytes;
}

// Whether a wrap-aware (unsigned32) `serial` lies within `span` of the window
// [lo, hi]. Far-out serials drive allocation/iteration proportional to their
// distance from the window, so the channel rejects them before acting (N-SEC-12).
// Deltas are computed as signed 32-bit so serial wrap is handled correctly.
inline bool SerialWithinSpan(uint32_t serial, uint32_t lo, uint32_t hi, int64_t span)
{
    const int64_t ahead = static_cast<int64_t>(static_cast<int32_t>(serial - hi));
    const int64_t behind = static_cast<int64_t>(static_cast<int32_t>(lo - serial));
    return ahead <= span && behind <= span;
}

// A wire array element count is acceptable to decode: non-negative and no larger
// than the bytes left in the buffer. Each element consumes at least one byte, so
// a count that exceeds the remaining bytes is impossible and a crafted huge count
// can neither overflow the size computation nor force an out-of-band allocation
// (N-SEC-05). This buffer bound subsumes any fixed element cap for UDP-sized packets.
inline bool DecodeCountFits(int count, int remainingBytes)
{
    return count >= 0 && count <= remainingBytes;
}

// A file-transfer segment is geometrically valid against the declared totals:
// all sizes non-negative, the segment index inside the segment count, and the
// byte range inside the file. Guards the ReceiveFileSegment OOB write (N-SEC-04).
inline bool SegmentInBounds(int totSize, int totSegments, int curSegment, int offset, int dataSize)
{
    if (totSize < 0 || totSegments < 0 || curSegment < 0 || offset < 0 || dataSize < 0)
    {
        return false;
    }
    if (curSegment >= totSegments)
    {
        return false;
    }
    return RangeInBounds(offset, dataSize, totSize);
}

// A wire-supplied name is a well-formed, length-bounded identifier: non-empty, at
// most `maxLen` chars, first char a letter or underscore, the rest alphanumeric or
// underscore. Used to reject malformed/oversized publicVariable names before they
// reach the script var table (N-SEC-15); it does not (and cannot, in the engine)
// decide whether a legitimately-named mission variable is one a client may set.
inline bool ValidIdentifier(const char* s, int maxLen)
{
    if (!s || s[0] == '\0' || maxLen <= 0)
    {
        return false;
    }
    auto isAlpha = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; };
    auto isAlnum = [&](char c) { return isAlpha(c) || (c >= '0' && c <= '9'); };
    if (!isAlpha(s[0]))
    {
        return false;
    }
    for (int i = 0; s[i] != '\0'; ++i)
    {
        if (i >= maxLen)
        {
            return false;
        }
        if (!isAlnum(s[i]))
        {
            return false;
        }
    }
    return true;
}

} // namespace WireBounds

} // namespace Poseidon
