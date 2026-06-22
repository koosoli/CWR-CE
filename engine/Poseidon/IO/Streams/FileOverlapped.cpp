
#include <Poseidon/IO/Streams/FileOverlapped.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>

#ifdef _WIN32

namespace Poseidon
{
void CALLBACK FileBufferOverlappedCompleted(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
                                            LPOVERLAPPED lpOverlapped)
{
    // Canceled operations are handled by the caller of CancelIo().
    if (dwErrorCode == ERROR_OPERATION_ABORTED)
    {
        return;
    }

    FileBufferOverlapped* fbo = (FileBufferOverlapped*)lpOverlapped->hEvent;
    fbo->CompletedCallback(dwErrorCode, dwNumberOfBytesTransfered);
}

void FileBufferOverlapped::CompletedCallback(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered) const
{
    _completed = true;
    if (dwErrorCode != 0 || dwNumberOfBytesTransfered != static_cast<DWORD>(_buffer.Size()))
    {
        _buffer.Delete();
    }
}

void FileBufferOverlapped::Open(HANDLE fileHandle, int start = 0, int size = INT_MAX)
{
    _fileHandle = fileHandle;
    int fileSize = ::GetFileSize(fileHandle, nullptr);

    saturate(start, 0, fileSize);
    saturate(size, 0, fileSize - start);

    // start overlapped reading
    _buffer.Resize(size);
    if (!_compressed)
    {
        _uncompressedSize = size;
    }

    _data.Offset = start;
    _data.OffsetHigh = 0;
    _data.hEvent = this;

    BOOL ok = ReadFileEx(_fileHandle, _buffer.Data(), _buffer.Size(), &_data, FileBufferOverlappedCompleted);
    if (!ok)
    {
        _completed = true;
        _buffer.Release();
    }
}

FileBufferOverlapped::FileBufferOverlapped(HANDLE file, int uncompressedSize, int start, int size)
{
    _size = 0;
    _fileHandle = nullptr;
    _completed = false;
    _compressed = true;
    _uncompressedSize = uncompressedSize;

    if (size <= 0)
    {
        return; // zero sized - no data
    }

    Open(file, start, size);
}

FileBufferOverlapped::FileBufferOverlapped(HANDLE file, int start, int size)
{
    _size = 0;
    _fileHandle = nullptr;
    _completed = false;
    _compressed = false;
    _uncompressedSize = size;

    if (size <= 0)
    {
        return; // zero sized - no data
    }

    Open(file, start, size);
}

bool FileBufferOverlapped::IsDone() const
{
    if (!_completed)
    {
        SleepEx(0, TRUE);
    }
    return _completed;
}

int FileBufferOverlapped::GetSize() const
{
    if (_compressed)
    {
        return _uncompressedSize;
    }
    return _buffer.Size();
}

const char* FileBufferOverlapped::GetData() const
{
    // wait until overlapped function completed
    while (!_completed)
    {
        SleepEx(0, TRUE);
    }
    if (_compressed)
    {
        Buffer<char> uncompressed;
        uncompressed.Resize(_uncompressedSize);
        SSCompress ss;
        QIStream in(_buffer.Data(), _buffer.Size());
        ss.Decode(uncompressed.Data(), uncompressed.Size(), in);
        _buffer.Init(uncompressed.Data(), uncompressed.Size());
    }
    return _buffer.Data();
}

FileBufferOverlapped::~FileBufferOverlapped()
{
    CancelIo(_fileHandle);
    _buffer.Release();
    _completed = true;
}

bool FileBufferOverlapped::GetError() const
{
    return _buffer.Size() == 0;
}

bool FileBufferOverlapped::IsReady() const
{
    return IsDone();
}

bool FileBufferOverlapped::IsFromBank(QFBank* bank) const
{
    return bank->BufferOwned(this);
}

HANDLE FileBufferOverlapped::GetFileHandle() const
{
    return _fileHandle;
}

} // namespace Poseidon

#endif // _WIN32
