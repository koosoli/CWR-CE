#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_OVERLAPPED_HPP
#define _FILE_OVERLAPPED_HPP

#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Foundation/Common/Win.h>

#ifdef _WIN32


namespace Poseidon
{
class FileBufferOverlapped : public IFileBuffer
{
    // we did not open handle - we just remmember it
    mutable Buffer<char> _buffer;
    //! handle (must be crated with FILE_FLAG_OVERLAPPED
    HANDLE _fileHandle;
    //! size to read
    int _size;
    //! size after decompression
    int _uncompressedSize;

    //! set by CompletedCallback when completed
    mutable bool _completed;
    //! determine if decompression should be done after read
    bool _compressed;
    //! overlapped data structure for ReadFileEx
    OVERLAPPED _data;

    void Open(HANDLE fileHandle, int start, int size);

  public:
    FileBufferOverlapped(HANDLE file, int start = 0, int size = INT_MAX);
    FileBufferOverlapped(HANDLE file, int uncompressedSize, int start, int size);
    ~FileBufferOverlapped() override;

    //! get data; if IO is pending, blocks until it is finished
    const char* GetData() const override;
    int GetSize() const override;
    bool GetError() const override;
    bool IsFromBank(QFBank* bank) const override;
    //! check if overlapped I/O is finished
    bool IsReady() const override;
    HANDLE GetFileHandle() const;
    //! called when overlapped I/O is finished
    void CompletedCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered) const;
    bool IsDone() const;
};

} // namespace Poseidon

#endif // _WIN32

#endif
