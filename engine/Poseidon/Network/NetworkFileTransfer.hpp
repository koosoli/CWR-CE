#pragma once

#include <algorithm>
#include <cstring>

#include <Poseidon/Foundation/Strings/RString.hpp>

// Keep mission/file transfer payloads comfortably below the transport's ~1490-byte data ceiling
// after message metadata/path overhead is added.

namespace Poseidon
{

constexpr int NetworkFileTransferSegmentSize = 1024;

inline int GetNetworkFileTransferSegmentCount(int totalSize, int maxSegmentSize = NetworkFileTransferSegmentSize)
{
    if (totalSize <= 0)
    {
        return 0;
    }

    return (totalSize + maxSegmentSize - 1) / maxSegmentSize;
}

inline bool ApplyReceivedNetworkFileTransferSegment(bool& segmentReceived, int segmentSize, int& receivedBytes,
                                                    int& remainingSegments)
{
    if (!segmentReceived)
    {
        segmentReceived = true;
        receivedBytes += segmentSize;
        --remainingSegments;
    }

    return remainingSegments <= 0;
}

template <typename MessageType, typename SendFn>
int SendNetworkFileTransferSegments(const RString& destinationPath, const void* data, int totalSize,
                                    SendFn&& sendSegment, int maxSegmentSize = NetworkFileTransferSegmentSize)
{
    MessageType msg;
    msg.path = destinationPath;
    msg.totSize = totalSize;
    msg.totSegments = GetNetworkFileTransferSegmentCount(totalSize, maxSegmentSize);
    msg.offset = 0;

    const char* bytes = static_cast<const char*>(data);
    for (int i = 0; i < msg.totSegments; i++)
    {
        msg.curSegment = i;
        const int size = std::min(maxSegmentSize, totalSize - msg.offset);
        msg.data.Resize(size);
        memcpy(msg.data.Data(), bytes + msg.offset, size);
        sendSegment(msg);
        msg.offset += size;
    }

    return msg.totSegments;
}

} // namespace Poseidon
