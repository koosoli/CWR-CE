#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_MAPPING_HPP
#define _FILE_MAPPING_HPP

#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Foundation/Common/Win.h>


namespace Poseidon
{
class FileBufferMapped : public IFileBuffer
{
    HANDLE _fileHandle;
#ifdef _WIN32
    HANDLE _mapHandle; // not used in POSIX
#endif
    void* _view;
    int _size;

    void Open(HANDLE fileHandle, int start, int size);

  public:
    FileBufferMapped(HANDLE file, int start = 0, int size = INT_MAX);
    FileBufferMapped(const char* name, int start = 0, int size = INT_MAX);
    ~FileBufferMapped();

    const char* GetData() const { return (char*)_view; }
    int GetSize() const { return _size; }
    bool GetError() const;
    bool IsFromBank(QFBank* bank) const;
    bool IsReady() const;
    HANDLE GetFileHandle() const;
};

#endif
} // namespace Poseidon
