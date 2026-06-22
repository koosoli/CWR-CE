#ifdef _MSC_VER
#pragma once
#endif

#ifndef _FILE_COMPRESS_HPP
#define _FILE_COMPRESS_HPP

#include <Poseidon/IO/Streams/QStream.hpp>


namespace Poseidon
{
class FileBufferUncompressed : public FileBufferMemory
{
  public:
    FileBufferUncompressed(int outSize, QIStream& in);
};

class FileBufferSub : public IFileBuffer
{
    Ref<IFileBuffer> _whole;
    int _start, _size;

  public:
    FileBufferSub(IFileBuffer* buf, int start, int size);

    const char* GetData() const override { return _whole->GetData() + _start; }
    int GetSize() const override { return _size; }
    bool GetError() const override { return _whole->GetError(); }
    bool IsFromBank(QFBank* bank) const override { return _whole->IsFromBank(bank); }
    bool IsReady() const override { return true; }
};

} // namespace Poseidon
#endif
