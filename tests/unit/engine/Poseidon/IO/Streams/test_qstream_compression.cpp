#include <catch2/catch_test_macros.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/IO/Streams/SerializeBin.hpp>
#include "../Support/test_fixtures.hpp"
#include <cstring>

// QStream Compression Tests - SSCompress
// Tests for compression and decompression functionality

using namespace TestFixtures;
using namespace Poseidon;

TEST_CASE("SerializeBinStream string read stops at EOF", "[qstream][serializebin][fuzz]")
{
    // fuzz_wrp: operator<<(RString&)/(RStringB&) read chars until a NUL byte, but
    // get() returns EOF (-1, which as a char is 0xFF != 0) so an unterminated string
    // spun forever. The loop now stops at EOF -- the test completing IS the regression.
    char data[] = {'a', 'b', 'c'}; // no NUL terminator before EOF
    QIStream f(data, 3);
    SerializeBinStream sf(&f);
    RStringB s;
    sf << s;
    REQUIRE(strcmp(s.Data(), "abc") == 0);
}

TEST_CASE("SerializeBinStream TransferBasicArray rejects an oversized count", "[qstream][serializebin][fuzz]")
{
    // fuzz_wrp: a wire element count drove AutoArray::Realloc unchecked -> a huge or
    // negative allocation. The count is now rejected when the remaining stream cannot
    // back it (each element needs at least one byte).
    int huge = 0x40000000; // far more than the 4-byte buffer can supply
    char data[4];
    std::memcpy(data, &huge, sizeof(huge));
    QIStream f(data, sizeof(data));
    SerializeBinStream sf(&f);
    AutoArray<int> arr;
    sf.TransferBasicArray(arr);
    REQUIRE(sf.GetError() != SerializeBinStream::EOK);
    REQUIRE(arr.Size() == 0);
}

// Basic Compression Tests

TEST_CASE("SSCompress - Encode small text data", "[qstream][compression]")
{
    SECTION("Compress simple text string")
    {
        const char* input = "Hello World! This is a test string for compression.";
        int inputSize = strlen(input);

        SSCompress compressor;
        QOStream output;
        compressor.Encode(output, input, inputSize);

        // Compressed data should exist
        REQUIRE(output.pcount() > 0);

        // Compressed size should be less than or equal to input
        // (for small data, might be slightly larger due to overhead)
        REQUIRE(output.pcount() <= inputSize + 100);
    }
}

TEST_CASE("SSCompress - Decode compressed data", "[qstream][compression]")
{
    SECTION("Round-trip compress and decompress")
    {
        const char* original = "The quick brown fox jumps over the lazy dog.";
        int originalSize = strlen(original);

        SSCompress compressor;

        // Compress
        QOStream compressed;
        compressor.Encode(compressed, original, originalSize);

        // Decompress
        QIStream compressedInput(compressed.str(), compressed.pcount());
        char decompressed[100];

        bool result = compressor.Decode(decompressed, originalSize, compressedInput);

        REQUIRE(result == true);
        REQUIRE(memcmp(original, decompressed, originalSize) == 0);
    }
}

TEST_CASE("SSCompress - Empty data", "[qstream][compression]")
{
    SECTION("Compress empty string")
    {
        const char* input = "";
        int inputSize = 0;

        SSCompress compressor;
        QOStream output;
        compressor.Encode(output, input, inputSize);

        // Should handle empty data gracefully
        REQUIRE(output.pcount() >= 0);
    }
}

TEST_CASE("SSCompress - Repetitive data compression", "[qstream][compression]")
{
    SECTION("Compress highly repetitive data")
    {
        char input[200];
        memset(input, 'A', sizeof(input));

        SSCompress compressor;
        QOStream output;
        compressor.Encode(output, input, sizeof(input));

        // Repetitive data should compress well
        REQUIRE(output.pcount() < static_cast<int>(sizeof(input)));
        REQUIRE(output.pcount() > 0);
    }
}

TEST_CASE("SSCompress - Binary data", "[qstream][compression]")
{
    SECTION("Compress binary pattern")
    {
        char input[256];
        for (int i = 0; i < 256; i++)
        {
            input[i] = static_cast<char>(i);
        }

        SSCompress compressor;
        QOStream compressed;
        compressor.Encode(compressed, input, sizeof(input));

        REQUIRE(compressed.pcount() > 0);

        // Decompress
        QIStream compressedInput(compressed.str(), compressed.pcount());
        char decompressed[256];

        bool result = compressor.Decode(decompressed, sizeof(input), compressedInput);

        REQUIRE(result == true);
        REQUIRE(memcmp(input, decompressed, sizeof(input)) == 0);
    }
}

TEST_CASE("SSCompress - Large data", "[qstream][compression]")
{
    SECTION("Compress 1KB of data")
    {
        const int size = 1024;
        char input[size];

        // Fill with pattern
        for (int i = 0; i < size; i++)
        {
            input[i] = static_cast<char>(i % 256);
        }

        SSCompress compressor;
        QOStream compressed;
        compressor.Encode(compressed, input, size);

        REQUIRE(compressed.pcount() > 0);

        // Decompress
        QIStream compressedInput(compressed.str(), compressed.pcount());
        char decompressed[size];

        bool result = compressor.Decode(decompressed, size, compressedInput);

        REQUIRE(result == true);
        REQUIRE(memcmp(input, decompressed, size) == 0);
    }
}

TEST_CASE("SSCompress - Multiple compression cycles", "[qstream][compression]")
{
    SECTION("Compress same data multiple times")
    {
        const char* input = "Test data for multiple compressions";
        int inputSize = strlen(input);

        SSCompress compressor1, compressor2;
        QOStream output1, output2;

        compressor1.Encode(output1, input, inputSize);
        compressor2.Encode(output2, input, inputSize);

        // Same input should produce same compressed output
        REQUIRE(output1.pcount() == output2.pcount());
        REQUIRE(memcmp(output1.str(), output2.str(), output1.pcount()) == 0);
    }
}

TEST_CASE("SSCompress - Compression ratio verification", "[qstream][compression]")
{
    SECTION("Verify compression actually reduces size")
    {
        // Create highly compressible data
        const int size = 500;
        char input[size];

        // Fill with repeating pattern (very compressible)
        for (int i = 0; i < size; i++)
        {
            input[i] = "ABCD"[i % 4];
        }

        SSCompress compressor;
        QOStream compressed;
        compressor.Encode(compressed, input, size);

        // Should achieve significant compression
        REQUIRE(compressed.pcount() < size);
        REQUIRE(compressed.pcount() < size / 2); // At least 50% compression
    }
}
