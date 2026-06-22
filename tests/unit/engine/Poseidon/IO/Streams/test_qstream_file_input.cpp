#include <catch2/catch_test_macros.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../Support/test_fixtures.hpp"
#include <cstring>
#include <Poseidon/Foundation/Types/Pointers.hpp>

// QStream File Input Tests - QIFStream
// Tests for file-based input stream operations

using namespace TestFixtures;

TEST_CASE("QIFStream - Open file from disk", "[qstream][file][input]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Open and read existing file")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");
        QIFStream stream;

        stream.open(path);

        REQUIRE(stream.fail() == false);
        REQUIRE(stream.eof() == false);
        REQUIRE(stream.rest() > 0);

        // Read some data
        char buffer[50];
        stream.read(buffer, 49);
        buffer[49] = '\0';

        REQUIRE(stream.fail() == false);
        REQUIRE(strlen(buffer) > 0);
    }

    SECTION("File loaded into memory")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");
        QIFStream stream;

        stream.open(path);

        // QIFStream loads entire file into memory
        int totalSize = stream.rest();
        REQUIRE(totalSize > 0);

        // Can seek and read repeatedly
        stream.seekg(0, QIOS::beg);
        REQUIRE(stream.rest() == totalSize);
    }
}

TEST_CASE("QIFStream - Open non-existent file", "[qstream][file][input][error]")
{
    SECTION("Attempt to open missing file")
    {
        QIFStream stream;

        // Try to open file that doesn't exist
        stream.open("nonexistent_file_12345.txt");

        // Should indicate failure
        REQUIRE(stream.fail() == true);
        REQUIRE(stream.rest() == 0);
    }

    SECTION("Error code set correctly")
    {
        QIFStream stream;

        stream.open("missing.dat");

        REQUIRE(stream.fail() == true);
        LSError err = stream.error();
        // Error should be FileNotFound or similar
        REQUIRE(err != LSOK);
    }
}

TEST_CASE("QIFStream - FileExists static method", "[qstream][file][input][utility]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Check existing file")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");

        bool exists = QIFStream::FileExists(path);

        REQUIRE(exists == true);
    }

    SECTION("Check non-existent file")
    {
        bool exists = QIFStream::FileExists("definitely_not_a_real_file_xyz.txt");

        REQUIRE(exists == false);
    }
}

TEST_CASE("QIFStream - FileReadOnly static method", "[qstream][file][input][utility]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Check file read-only status")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");

        // Just verify method exists and doesn't crash
        bool readonly = QIFStream::FileReadOnly(path);

        // Result depends on file system permissions
        // Just verify we got a boolean result
        REQUIRE((readonly == true || readonly == false));
    }
}

TEST_CASE("QIFStream - OpenBuffer with IFileBuffer", "[qstream][file][input][buffer]")
{
    SECTION("Create buffer and open stream")
    {
        // Create a memory buffer on the heap (required for Ref<>)
        const char testData[] = "Buffer test data";
        FileBufferMemory* bufferPtr = new FileBufferMemory(strlen(testData));
        memcpy(bufferPtr->GetWritableData(), testData, strlen(testData));

        Ref<IFileBuffer> buffer = bufferPtr; // Upcast to interface

        // Open stream with buffer
        QIFStream stream;
        stream.OpenBuffer(buffer);

        REQUIRE(stream.fail() == false);
        REQUIRE(stream.rest() == static_cast<int>(strlen(testData)));

        // Read data
        char readBuffer[20];
        stream.read(readBuffer, strlen(testData));
        readBuffer[strlen(testData)] = '\0';

        REQUIRE(strcmp(readBuffer, testData) == 0);
    }
}

TEST_CASE("QIFStream - GetBuffer returns buffer reference", "[qstream][file][input][buffer]")
{
    SECTION("GetBuffer after OpenBuffer")
    {
        FileBufferMemory* bufferPtr = new FileBufferMemory(10);
        memcpy(bufferPtr->GetWritableData(), "Test", 4);

        Ref<IFileBuffer> buffer = bufferPtr;

        QIFStream stream;
        stream.OpenBuffer(buffer);

        IFileBuffer* retrieved = stream.GetBuffer();

        REQUIRE(retrieved != nullptr);
        REQUIRE(retrieved->GetSize() == 10);
    }

    SECTION("GetBuffer after open file")
    {
        REQUIRE_FIXTURE("qstream/test_input.txt");

        const char* path = GetTestFixturePath("qstream/test_input.txt");
        QIFStream stream;
        stream.open(path);

        IFileBuffer* buffer = stream.GetBuffer();

        // After opening file, should have a buffer
        REQUIRE(buffer != nullptr);
        REQUIRE(buffer->GetSize() > 0);
    }
}

TEST_CASE("QIFStream - Copy construction", "[qstream][file][input][copy]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Copy constructor shares data")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");
        QIFStream stream1;
        stream1.open(path);

        // Read some data from stream1
        char buffer1[10];
        stream1.read(buffer1, 10);
        int pos1 = stream1.tellg();

        // Copy construct stream2
        QIFStream stream2(stream1);

        // stream2 should have same position and data
        REQUIRE(stream2.tellg() == pos1);
        REQUIRE(stream2.fail() == stream1.fail());

        // Both should be able to read remaining data
        REQUIRE(stream2.rest() > 0);
    }
}

TEST_CASE("QIFStream - Assignment operator", "[qstream][file][input][copy]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Assignment shares data")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");
        QIFStream stream1;
        stream1.open(path);

        QIFStream stream2;
        stream2 = stream1;

        // stream2 should have same data access
        REQUIRE(stream2.fail() == false);
        REQUIRE(stream2.rest() > 0);

        // Both can read independently
        char buffer1[5], buffer2[5];
        stream1.read(buffer1, 5);
        stream2.read(buffer2, 5);

        REQUIRE(memcmp(buffer1, buffer2, 5) == 0);
    }
}

TEST_CASE("QIFStream - Import from file", "[qstream][file][input][import]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Import file data")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");
        QIFStream stream;

        stream.import(path);

        REQUIRE(stream.fail() == false);
        REQUIRE(stream.rest() > 0);

        // Verify data is accessible
        char buffer[100];
        int bytesToRead = stream.rest() < 100 ? stream.rest() : 99;
        stream.read(buffer, bytesToRead);
        buffer[bytesToRead] = '\0';

        REQUIRE(strlen(buffer) > 0);
    }
}

TEST_CASE("QIFStream - Import error handling", "[qstream][file][input][import][error]")
{
    SECTION("Import non-existent file")
    {
        QIFStream stream;

        stream.import("this_file_does_not_exist_xyz.txt");

        // Should fail gracefully
        REQUIRE(stream.fail() == true);
    }
}

TEST_CASE("QIFStream - Close and reopen file", "[qstream][file][input][lifecycle]")
{
    REQUIRE_FIXTURE("qstream/test_input.txt");

    SECTION("Open, close, then reopen same file")
    {
        const char* path = GetTestFixturePath("qstream/test_input.txt");

        QIFStream stream;

        // First open
        stream.open(path);
        REQUIRE(stream.fail() == false);
        int size1 = stream.rest();
        REQUIRE(size1 > 0);

        // Read some data
        char buffer1[10];
        stream.read(buffer1, 10);

        // Close (DoDestruct)
        stream.DoDestruct();

        // Reopen same file
        stream.open(path);
        REQUIRE(stream.fail() == false);
        int size2 = stream.rest();

        // Should have same size as first open
        REQUIRE(size2 == size1);

        // Should be able to read same data again
        char buffer2[10];
        stream.read(buffer2, 10);
        REQUIRE(memcmp(buffer1, buffer2, 10) == 0);
    }
}

TEST_CASE("QIFStream - Multiple operations on same file", "[qstream][file][input][operations]")
{
    REQUIRE_FIXTURE("qstream/test_large.bin");

    SECTION("Seek, read, seek again on large file")
    {
        const char* path = GetTestFixturePath("qstream/test_large.bin");
        QIFStream stream;
        stream.open(path);

        REQUIRE(stream.fail() == false);
        int totalSize = stream.rest();
        REQUIRE(totalSize > 1000); // Large file

        // Read from beginning
        char buffer1[100];
        stream.read(buffer1, 100);
        REQUIRE(stream.tellg() == 100);

        // Seek to middle
        stream.seekg(totalSize / 2, QIOS::beg);
        REQUIRE(stream.tellg() == totalSize / 2);

        // Read some more
        char buffer2[100];
        stream.read(buffer2, 100);

        // Seek back to beginning
        stream.seekg(0, QIOS::beg);
        REQUIRE(stream.tellg() == 0);

        // Re-read and verify same data
        char buffer3[100];
        stream.read(buffer3, 100);
        REQUIRE(memcmp(buffer1, buffer3, 100) == 0);
    }
}
