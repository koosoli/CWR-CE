#pragma once

#include <Poseidon/IO/Streams/QStream.hpp>
#include <vector>
#include <string>
#include <cstring>

// Helper: Create QIStream from raw bytes

class TestQIStream : public QIStream
{
  public:
    TestQIStream(const std::vector<uint8_t>& data)
    {
        _len = static_cast<int>(data.size());
        _buf = new char[_len];
        std::memcpy(_buf, data.data(), _len);
        _readFrom = 0;
        _eof = false;
        _fail = false;
    }

    ~TestQIStream()
    {
        if (_buf)
            delete[] _buf;
    }
};

// Helper: Create QOStream for compression testing

class TestQOStream : public QOStream
{
  public:
    std::vector<uint8_t> data;

    TestQOStream() : QOStream()
    {
        // Base class initializes _buffer automatically
    }

    // Override write to capture data
    void write(const void* src, int len)
    {
        // Call base class to maintain internal state
        QOStream::write(src, len);
    }

    // Get the compressed data after encoding
    std::vector<uint8_t> getData() const
    {
        const char* buf = _buffer.Data();
        int size = _writeTo;
        return std::vector<uint8_t>(buf, buf + size);
    }
};

// Helper: Write test data

template <typename T>
inline void writeValue(std::vector<uint8_t>& buffer, T value)
{
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    buffer.insert(buffer.end(), bytes, bytes + sizeof(T));
}

inline void writeAsciiz(std::vector<uint8_t>& buffer, const std::string& str)
{
    buffer.insert(buffer.end(), str.begin(), str.end());
    buffer.push_back(0);
}

inline void writeString(std::vector<uint8_t>& buffer, const std::string& str)
{
    writeValue(buffer, static_cast<int32_t>(str.size()));
    buffer.insert(buffer.end(), str.begin(), str.end());
}

// Helper: Compress data using SSCompress

inline std::vector<uint8_t> compressData(const std::vector<uint8_t>& uncompressed)
{
    TestQOStream outStream;
    SSCompress compressor;
    compressor.Encode(outStream, reinterpret_cast<const char*>(uncompressed.data()),
                      static_cast<long>(uncompressed.size()));
    return outStream.getData();
}
