#ifdef _MSC_VER
#pragma once
#endif

#ifndef _SERIALIZE_BIN_HPP
#define _SERIALIZE_BIN_HPP

#include <Poseidon/Foundation/Strings/RString.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>


namespace Poseidon
{
class SerializeBinStream
{
  public:
    enum ErrorCode
    {
        EOK,
        EBadFileType,
        EBadVersion,
        EFileStructure,
        EGeneric
    };

  private:
    QIStream* _in;
    QOStream* _out;
    ErrorCode _error;
    void* _context;

  public:
    SerializeBinStream(QIStream* in);
    SerializeBinStream(QOStream* out);
    bool IsSaving() const { return _out != nullptr; }
    bool IsLoading() const { return _in != nullptr; }

    ErrorCode GetError() const { return _error; }
    void SetError(ErrorCode code) { _error = code; }

    void* GetContext() const { return _context; }
    void SetContext(void* context) { _context = context; }

    int TellG();
    void SeekG(int offset);

    // load/save helpers
    bool Version(int ver);
    void Load(void* data, int size) { _in->read(data, size); }
    int LoadInt()
    {
        int t = 0;
        _in->read(&t, sizeof(t));
        return t;
    }
    short LoadShort()
    {
        short t = 0;
        _in->read(&t, sizeof(t));
        return t;
    }
    char LoadChar() { return _in->get(); }

    int GetRest() { return _in ? _in->rest() : 1; }

    void Save(const void* data, int size) { _out->write(data, size); }
    void SaveInt(int t) { _out->write(&t, sizeof(t)); }
    void SaveShort(short t) { _out->write(&t, sizeof(t)); }
    void SaveChar(char t) { _out->put(t); }

    // generic helpers
    void TransferBinary(void* data, int size)
    {
        if (_in)
        {
            _in->read(data, size);
        }
        else
        {
            _out->write(data, size);
        }
    }
    void TransferBinaryCompressed(void* data, int size);
    void SaveCompressed(const void* data, int size);
    void LoadCompressed(void* data, int size);
#define BIN_TRANSFER(x) TransferBinary(&(x), sizeof(x))
#define BIN_TRANSFER_COMPRESS(x) TransferBinaryCompressed(&(x), sizeof(x))
    // transfer basic types
    void operator<<(int& data) { TransferBinary(&data, sizeof(data)); }
    void operator<<(float& data) { TransferBinary(&data, sizeof(data)); }
    void operator<<(bool& data) { TransferBinary(&data, sizeof(data)); }
    void operator<<(char& data) { TransferBinary(&data, sizeof(data)); }
    void operator<<(signed char& data) { TransferBinary(&data, sizeof(data)); }
    void operator<<(unsigned char& data) { TransferBinary(&data, sizeof(data)); }

    void operator<<(RString& data);
    void operator<<(RStringB& data);

    template <class Type>
    void Transfer(Type& val)
    {
        (*this) << val;
    }

    template <class ArrayType>
    void TransferBinaryArray(ArrayType& data)
    {
        if (_in)
        {
            int size = LoadInt();
            // A negative count makes Realloc(size) call MemAlloc with a near-2^64 size_t
            // (Resize clamps negatives but Realloc does not). The payload is compressed, so
            // the count can exceed the remaining bytes — but the buffer is size*element, and a
            // count whose buffer would be an absurd allocation is malformed regardless; reject
            // it before Realloc drives a multi-GB OOM / allocation-size-too-big.
            const long long binBytes = static_cast<long long>(size) * static_cast<long long>(sizeof(*data.Data()));
            if (size < 0 || binBytes > (256LL << 20)) // 256 MB — far above any real model array
            {
                _error = EFileStructure;
                return;
            }
            data.Realloc(size);
            data.Resize(size);
        }
        else
        {
            SaveInt(data.Size());
        }
        TransferBinaryCompressed(data.Data(), data.Size() * sizeof(*data.Data()));
    }

    template <class ArrayType>
    void TransferBasicArray(ArrayType& data)
    {
        if (_in)
        {
            int size = LoadInt();
            // Each element is transferred uncompressed below, so it needs at least one
            // byte of input; reject a count that cannot be backed by the remaining
            // stream before it drives a huge/negative Realloc.
            if (size < 0 || size > GetRest())
            {
                _error = EFileStructure;
                return;
            }
            data.Realloc(size);
            data.Resize(size);
        }
        else
        {
            SaveInt(data.Size());
        }
        for (int i = 0; i < data.Size(); i++)
        {
            Transfer(data[i]);
        }
    }
    template <class ArrayType>
    void TransferArray(ArrayType& data)
    {
        if (_in)
        {
            int size = LoadInt();
            // Elements are transferred uncompressed below; reject a count that cannot
            // be backed by the remaining stream before a huge/negative Realloc.
            if (size < 0 || size > GetRest())
            {
                _error = EFileStructure;
                return;
            }
            data.Realloc(size);
            data.Resize(size);
        }
        else
        {
            SaveInt(data.Size());
        }
        for (int i = 0; i < data.Size(); i++)
        {
            data[i].SerializeBin(*this);
        }
    }

    template <class Type, class ArrayType>
    void TransferRefArrayT(ArrayType& data, Type* type)
    {
        if (_in)
        {
            int size = LoadInt();
            // Each element is `new`-ed and then reads from the stream, so a count that
            // cannot be backed by the remaining bytes is malformed — reject before a
            // huge/negative Realloc and a count-driven allocation loop (OOM).
            if (size < 0 || size > GetRest())
            {
                _error = EFileStructure;
                return;
            }
            data.Realloc(size);
            data.Resize(size);
            for (int i = 0; i < data.Size(); i++)
            {
                data[i] = new Type;
            }
        }
        else
        {
            SaveInt(data.Size());
        }
        for (int i = 0; i < data.Size(); i++)
        {
            data[i]->SerializeBin(*this);
        }
    }
    template <class Type>
    void TransferRefArray(RefArray<Type>& data)
    {
        TransferRefArrayT(data, (Type*)nullptr);
    }
};

} // namespace Poseidon

#endif
