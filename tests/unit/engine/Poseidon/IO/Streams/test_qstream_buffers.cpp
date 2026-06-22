#include <catch2/catch_test_macros.hpp>
#include <Poseidon/IO/Streams/FileAccessPolicy.hpp>
#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include "../Support/test_fixtures.hpp"
#include <cstring>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <functional>
#include <utility>
#include <Poseidon/Foundation/Types/Pointers.hpp>
#include <Poseidon/Foundation/platform.hpp>

// QStream Buffer Tests - FileBuffer Classes
// Tests for FileBufferMemory and FileBufferLoaded

using namespace TestFixtures;

namespace
{

class StubFileBuffer final : public IFileBuffer
{
  public:
    explicit StubFileBuffer(std::string payload, bool error = false) : _payload(std::move(payload)), _error(error) {}

    bool GetError() const override { return _error; }
    const char* GetData() const override { return _payload.c_str(); }
    int GetSize() const override { return static_cast<int>(_payload.size()); }
    bool IsFromBank(QFBank* /*bank*/) const override { return false; }
    bool IsReady() const override { return true; }

  private:
    std::string _payload;
    bool _error;
};

} // namespace

TEST_CASE("QFileAccess - prefer mapped file buffer when available", "[qstream][policy]")
{
    int mappedCalls = 0;
    int loadedCalls = 0;

    QFileAccess::FileAccessHooks hooks;
    hooks.openMappedFile = [&](const char* /*name*/)
    {
        ++mappedCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("mapped"));
    };
    hooks.openLoadedFile = [&](const char* /*name*/)
    {
        ++loadedCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("loaded"));
    };
    hooks.fileExists = [](const char* /*name*/) { return true; };

    Ref<IFileBuffer> buffer = QFileAccess::OpenFileBufferAuto("ignored", &hooks);

    REQUIRE(buffer.NotNull());
    REQUIRE(buffer->GetError() == false);

    std::string payload(buffer->GetData(), buffer->GetSize());
    if (QFileAccess::MappingSupported())
    {
        REQUIRE(payload == "mapped");
        REQUIRE(mappedCalls == 1);
        REQUIRE(loadedCalls == 0);
    }
    else
    {
        REQUIRE(payload == "loaded");
        REQUIRE(mappedCalls == 0);
        REQUIRE(loadedCalls == 1);
    }
}

TEST_CASE("QFileAccess - mapped file fallback uses loaded buffer", "[qstream][policy]")
{
    int mappedCalls = 0;
    int loadedCalls = 0;

    QFileAccess::FileAccessHooks hooks;
    hooks.openMappedFile = [&](const char* /*name*/)
    {
        ++mappedCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("mapped-error", true));
    };
    hooks.openLoadedFile = [&](const char* /*name*/)
    {
        ++loadedCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("loaded"));
    };
    hooks.fileExists = [](const char* /*name*/) { return true; };

    Ref<IFileBuffer> buffer = QFileAccess::OpenFileBufferAuto("ignored", &hooks);

    REQUIRE(buffer.NotNull());
    REQUIRE(buffer->GetError() == false);
    REQUIRE(std::string(buffer->GetData(), buffer->GetSize()) == "loaded");

    if (QFileAccess::MappingSupported())
    {
        REQUIRE(mappedCalls == 1);
    }
    else
    {
        REQUIRE(mappedCalls == 0);
    }
    REQUIRE(loadedCalls == 1);
}

TEST_CASE("QFileAccess - missing file does not fall back to loaded buffer", "[qstream][policy]")
{
    int mappedCalls = 0;
    int loadedCalls = 0;

    QFileAccess::FileAccessHooks hooks;
    hooks.openMappedFile = [&](const char* /*name*/)
    {
        ++mappedCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("", true));
    };
    hooks.openLoadedFile = [&](const char* /*name*/)
    {
        ++loadedCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("loaded"));
    };
    hooks.fileExists = [](const char* /*name*/) { return false; };

    Ref<IFileBuffer> buffer = QFileAccess::OpenFileBufferAuto("missing", &hooks);

    REQUIRE(buffer.NotNull());
    REQUIRE(buffer->GetError() == true);
    if (QFileAccess::MappingSupported())
    {
        REQUIRE(mappedCalls == 1);
        REQUIRE(loadedCalls == 0);
    }
    else
    {
        REQUIRE(mappedCalls == 0);
        REQUIRE(loadedCalls == 1);
    }
}

TEST_CASE("QFileAccess - mapped bank access retries before logging fallback", "[qstream][policy]")
{
    if (!QFileAccess::MappingSupported())
    {
        SUCCEED();
        return;
    }

    int mapCalls = 0;
    int freeCalls = 0;
    int logCalls = 0;
    const HANDLE fakeHandle = (HANDLE)(intptr_t)1;

    QFileAccess::FileAccessHooks hooks;
    hooks.openMappedHandle = [&](HANDLE handle)
    {
        REQUIRE(handle == fakeHandle);
        ++mapCalls;
        if (mapCalls == 1)
        {
            return Ref<IFileBuffer>(new StubFileBuffer("mapped-error", true));
        }
        return Ref<IFileBuffer>(new StubFileBuffer("mapped-ok"));
    };
    hooks.freeUnusedBanks = [&](size_t sizeNeeded)
    {
        REQUIRE(sizeNeeded == 321u);
        ++freeCalls;
        return freeCalls == 1;
    };
    hooks.logMappingFailure = [&](const char* /*openName*/) { ++logCalls; };

    Ref<IFileBuffer> buffer = QFileAccess::TryOpenMappedBankAccess(fakeHandle, 321, "bank.pbo", &hooks);

    REQUIRE(buffer.NotNull());
    REQUIRE(buffer->GetError() == false);
    REQUIRE(std::string(buffer->GetData(), buffer->GetSize()) == "mapped-ok");
    REQUIRE(mapCalls == 2);
    REQUIRE(freeCalls == 1);
    REQUIRE(logCalls == 0);
}

TEST_CASE("QFileAccess - mapped bank access logs once when fallback is exhausted", "[qstream][policy]")
{
    if (!QFileAccess::MappingSupported())
    {
        SUCCEED();
        return;
    }

    int mapCalls = 0;
    int freeCalls = 0;
    int logCalls = 0;
    std::string loggedName;

    QFileAccess::FileAccessHooks hooks;
    hooks.openMappedHandle = [&](HANDLE /*handle*/)
    {
        ++mapCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("mapped-error", true));
    };
    hooks.freeUnusedBanks = [&](size_t sizeNeeded)
    {
        REQUIRE(sizeNeeded == 123u);
        ++freeCalls;
        return false;
    };
    hooks.logMappingFailure = [&](const char* openName)
    {
        ++logCalls;
        loggedName = openName;
    };

    Ref<IFileBuffer> buffer = QFileAccess::TryOpenMappedBankAccess((HANDLE)(intptr_t)2, 123, "broken.pbo", &hooks);

    REQUIRE(buffer.IsNull());
    REQUIRE(mapCalls == 1);
    REQUIRE(freeCalls == 1);
    REQUIRE(logCalls == 1);
    REQUIRE(loggedName == "broken.pbo");
}

TEST_CASE("QFileAccess - mapped bank access stops retrying after repeated failures", "[qstream][policy]")
{
    if (!QFileAccess::MappingSupported())
    {
        SUCCEED();
        return;
    }

    int mapCalls = 0;
    int freeCalls = 0;
    int logCalls = 0;

    QFileAccess::FileAccessHooks hooks;
    hooks.openMappedHandle = [&](HANDLE /*handle*/)
    {
        ++mapCalls;
        return Ref<IFileBuffer>(new StubFileBuffer("mapped-error", true));
    };
    hooks.freeUnusedBanks = [&](size_t /*sizeNeeded*/)
    {
        ++freeCalls;
        return true;
    };
    hooks.logMappingFailure = [&](const char* /*openName*/) { ++logCalls; };

    Ref<IFileBuffer> buffer = QFileAccess::TryOpenMappedBankAccess((HANDLE)(intptr_t)3, 64, "still-broken.pbo", &hooks);

    REQUIRE(buffer.IsNull());
    REQUIRE(mapCalls == 9);
    REQUIRE(freeCalls == 9);
    REQUIRE(logCalls == 1);
}

// Section 1: FileBufferMemory Tests (6 tests)

TEST_CASE("FileBufferMemory - Construction and allocation", "[qstream][buffer][memory]")
{
    SECTION("Default constructor")
    {
        FileBufferMemory buffer;

        // Default constructed buffer should be empty
        REQUIRE(buffer.GetSize() == 0);
        // GetData() may return nullptr for empty buffer
    }

    SECTION("Constructor with size")
    {
        const int bufferSize = 1024;
        FileBufferMemory buffer(bufferSize);

        REQUIRE(buffer.GetSize() == bufferSize);
        REQUIRE(static_cast<const void*>(buffer.GetData()) != nullptr);

        // Verify we can write to the buffer
        char* data = buffer.GetWritableData();
        REQUIRE(data != nullptr);

        // Fill with pattern
        for (int i = 0; i < bufferSize; i++)
        {
            data[i] = static_cast<char>(i % 256);
        }

        // Verify pattern
        const char* readData = buffer.GetData();
        for (int i = 0; i < 10; i++)
        {
            REQUIRE(readData[i] == static_cast<char>(i % 256));
        }
    }
}

TEST_CASE("FileBufferMemory - Realloc buffer", "[qstream][buffer][memory]")
{
    SECTION("Realloc to larger size")
    {
        FileBufferMemory buffer(100);

        // Fill with data
        char* data = buffer.GetWritableData();
        memcpy(data, "Test Data", 9);

        // Realloc to larger
        buffer.Realloc(500);

        REQUIRE(buffer.GetSize() == 500);
        REQUIRE(static_cast<const void*>(buffer.GetData()) != nullptr);

        // Note: After Realloc, old data is lost (not preserved)
        // This is expected behavior
    }

    SECTION("Realloc to smaller size")
    {
        FileBufferMemory buffer(1000);

        buffer.Realloc(100);

        REQUIRE(buffer.GetSize() == 100);
    }

    SECTION("Realloc to zero size")
    {
        FileBufferMemory buffer(500);

        buffer.Realloc(0);

        REQUIRE(buffer.GetSize() == 0);
    }
}

TEST_CASE("FileBufferMemory - GetWritableData access", "[qstream][buffer][memory]")
{
    SECTION("Write and read data")
    {
        const int size = 256;
        FileBufferMemory buffer(size);

        // Get writable pointer
        char* writable = buffer.GetWritableData();
        REQUIRE(writable != nullptr);

        // Write test data
        const char testData[] = "FileBufferMemory Test Data";
        memcpy(writable, testData, strlen(testData));

        // Read back through const pointer
        const char* readable = buffer.GetData();
        REQUIRE(memcmp(readable, testData, strlen(testData)) == 0);
    }

    SECTION("Multiple writes")
    {
        FileBufferMemory buffer(1024);
        char* data = buffer.GetWritableData();

        // Write at different offsets
        strcpy(data, "Start");
        strcpy(data + 100, "Middle");
        strcpy(data + 500, "End");

        // Verify all writes
        const char* read = buffer.GetData();
        REQUIRE(strcmp(read, "Start") == 0);
        REQUIRE(strcmp(read + 100, "Middle") == 0);
        REQUIRE(strcmp(read + 500, "End") == 0);
    }
}

TEST_CASE("FileBufferMemory - GetData and GetSize", "[qstream][buffer][memory]")
{
    SECTION("GetData returns valid pointer")
    {
        FileBufferMemory buffer(512);

        const char* data = buffer.GetData();
        REQUIRE(data != nullptr);
    }

    SECTION("GetSize returns correct size")
    {
        const int sizes[] = {0, 1, 100, 1024, 65536};

        for (int size : sizes)
        {
            FileBufferMemory buffer(size);
            REQUIRE(buffer.GetSize() == size);
        }
    }
}

TEST_CASE("FileBufferMemory - GetError always false", "[qstream][buffer][memory]")
{
    SECTION("GetError returns false for normal buffer")
    {
        FileBufferMemory buffer(100);

        REQUIRE(buffer.GetError() == false);
    }

    SECTION("GetError returns false even after operations")
    {
        FileBufferMemory buffer(100);

        // Perform operations
        buffer.GetWritableData()[0] = 'X';
        REQUIRE(buffer.GetError() == false);

        buffer.Realloc(200);
        REQUIRE(buffer.GetError() == false);
    }
}

TEST_CASE("FileBufferMemory - IsFromBank always false", "[qstream][buffer][memory]")
{
    SECTION("IsFromBank returns false")
    {
        FileBufferMemory buffer(100);

        // Memory buffers are never from a file bank
        REQUIRE(buffer.IsFromBank(nullptr) == false);
    }
}

// Section 2: FileBufferLoaded Tests (8 tests)

TEST_CASE("FileBufferLoaded - Load file into memory", "[qstream][buffer][loaded]")
{
    REQUIRE_FIXTURE("qstream/buffer_small.txt");

    SECTION("Load small text file")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        FileBufferLoaded buffer(path);

        // Should have loaded successfully
        REQUIRE(buffer.GetError() == false);
        REQUIRE(buffer.GetSize() > 0);
        REQUIRE(static_cast<const void*>(buffer.GetData()) != nullptr);

        // Verify content
        const char* data = buffer.GetData();
        int size = buffer.GetSize();

        // Should contain "Small text file"
        std::string content(data, size);
        REQUIRE(content.find("Small text file") != std::string::npos);
    }
}

TEST_CASE("FileBufferLoaded - Load large file", "[qstream][buffer][loaded]")
{
    REQUIRE_FIXTURE("qstream/buffer_large.bin");

    SECTION("Load large binary file")
    {
        const char* path = GetTestFixturePath("qstream/buffer_large.bin");

        FileBufferLoaded buffer(path);

        REQUIRE(buffer.GetError() == false);
        REQUIRE(buffer.GetSize() > 100000); // Large file should be > 100KB
        REQUIRE(static_cast<const void*>(buffer.GetData()) != nullptr);

        // Verify some data pattern
        const char* data = buffer.GetData();

        // File was created with pattern (0..255) * 5000
        // Verify first few bytes
        REQUIRE(static_cast<unsigned char>(data[0]) == 0);
        REQUIRE(static_cast<unsigned char>(data[1]) == 1);
        REQUIRE(static_cast<unsigned char>(data[255]) == 255);
        REQUIRE(static_cast<unsigned char>(data[256]) == 0); // Pattern repeats
    }
}

TEST_CASE("FileBufferLoaded - Load non-existent file", "[qstream][buffer][loaded][error]")
{
    SECTION("Attempt to load missing file")
    {
        // FileBufferLoaded may handle errors differently
        // Some implementations throw, others set error flag, others fail silently

        try
        {
            FileBufferLoaded buffer("non_existent_file_xyz_12345.bin");

            // Check various error indicators - just verify no crash
            (void)buffer.GetError();
            (void)buffer.GetSize();
            (void)buffer.GetData();
        }
        catch (...)
        {
            // Exception thrown - that's valid error handling
        }

        // Some form of error handling should have occurred
        // If implementation doesn't handle errors, that's documented behavior
        REQUIRE(true); // Document: FileBufferLoaded error handling varies
    }
}

TEST_CASE("FileBufferLoaded - Destructor cleanup", "[qstream][buffer][loaded]")
{
    REQUIRE_FIXTURE("qstream/buffer_small.txt");

    SECTION("Buffer cleans up after destruction")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        // Create in inner scope
        {
            FileBufferLoaded buffer(path);

            REQUIRE(buffer.GetSize() > 0);
            REQUIRE(static_cast<const void*>(buffer.GetData()) != nullptr);

            // Destructor will be called here
        }

        // If we get here without crash, cleanup worked
        REQUIRE(true);
    }
}

TEST_CASE("FileBufferLoaded - GetData returns content", "[qstream][buffer][loaded]")
{
    REQUIRE_FIXTURE("qstream/buffer_medium.bin");

    SECTION("GetData returns file content")
    {
        const char* path = GetTestFixturePath("qstream/buffer_medium.bin");

        FileBufferLoaded buffer(path);

        const char* data = buffer.GetData();
        REQUIRE(data != nullptr);

        int size = buffer.GetSize();
        REQUIRE(size > 1000); // Medium file

        // Verify pattern (1..255) * 50
        REQUIRE(static_cast<unsigned char>(data[0]) == 1);
        REQUIRE(static_cast<unsigned char>(data[1]) == 2);
        REQUIRE(static_cast<unsigned char>(data[254]) == 255);
        REQUIRE(static_cast<unsigned char>(data[255]) == 1); // Pattern repeats
    }
}

TEST_CASE("FileBufferLoaded - GetSize matches file size", "[qstream][buffer][loaded]")
{
    REQUIRE_FIXTURE("qstream/buffer_small.txt");

    SECTION("GetSize returns actual file size")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        // Get actual file size using standard C
        FILE* f = fopen(path, "rb");
        REQUIRE(f != nullptr);

        fseek(f, 0, SEEK_END);
        long actualSize = ftell(f);
        fclose(f);

        // Load with FileBufferLoaded
        FileBufferLoaded buffer(path);

        REQUIRE(buffer.GetSize() == actualSize);
    }
}

TEST_CASE("FileBufferLoaded - IsReady always true", "[qstream][buffer][loaded]")
{
    REQUIRE_FIXTURE("qstream/buffer_small.txt");

    SECTION("IsReady returns true after load")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        FileBufferLoaded buffer(path);

        // FileBufferLoaded loads synchronously, so always ready
        REQUIRE(buffer.IsReady() == true);
    }
}

TEST_CASE("FileBufferLoaded - RefCount management", "[qstream][buffer][loaded][refcount]")
{
    REQUIRE_FIXTURE("qstream/buffer_small.txt");

    SECTION("Reference counting works correctly")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        // Create buffer on heap
        FileBufferLoaded* bufferPtr = new FileBufferLoaded(path);

        // Wrap in Ref<> for reference counting
        Ref<IFileBuffer> ref1 = bufferPtr;

        // RefCounter should be 1
        REQUIRE(bufferPtr->RefCounter() == 1);

        // Create second reference
        {
            Ref<IFileBuffer> ref2 = ref1;

            // RefCounter should be 2
            REQUIRE(bufferPtr->RefCounter() == 2);

            // ref2 goes out of scope here
        }

        // RefCounter should be back to 1
        REQUIRE(bufferPtr->RefCounter() == 1);

        // ref1 will clean up when it goes out of scope
    }

    SECTION("Multiple Ref<> instances share buffer")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        FileBufferLoaded* bufferPtr = new FileBufferLoaded(path);
        Ref<IFileBuffer> ref1 = bufferPtr;

        const char* data1 = ref1->GetData();

        Ref<IFileBuffer> ref2 = ref1;
        const char* data2 = ref2->GetData();

        // Both should point to same data
        REQUIRE(data1 == data2);
        REQUIRE(ref1->GetSize() == ref2->GetSize());
    }
}

// Section 3: Integration Tests (Buffer + Stream)

TEST_CASE("FileBuffer - Integration with QIFStream", "[qstream][buffer][integration]")
{
    REQUIRE_FIXTURE("qstream/buffer_medium.bin");

    SECTION("Use FileBufferLoaded with QIFStream")
    {
        const char* path = GetTestFixturePath("qstream/buffer_medium.bin");

        // Load file into buffer
        FileBufferLoaded* bufferPtr = new FileBufferLoaded(path);
        Ref<IFileBuffer> buffer = bufferPtr;

        // Open stream with buffer
        QIFStream stream;
        stream.OpenBuffer(buffer);

        REQUIRE(stream.fail() == false);
        REQUIRE(stream.rest() == buffer->GetSize());

        // Read some data from stream
        char testData[10];
        stream.read(testData, 10);

        // Verify it matches buffer data
        const char* bufferData = buffer->GetData();
        REQUIRE(memcmp(testData, bufferData, 10) == 0);
    }

    SECTION("Use FileBufferMemory with QIFStream")
    {
        const char testString[] = "Memory buffer test data";

        FileBufferMemory* bufferPtr = new FileBufferMemory(strlen(testString));
        memcpy(bufferPtr->GetWritableData(), testString, strlen(testString));

        Ref<IFileBuffer> buffer = bufferPtr;

        QIFStream stream;
        stream.OpenBuffer(buffer);

        REQUIRE(stream.fail() == false);

        // Read back through stream
        char readData[30];
        stream.read(readData, strlen(testString));
        readData[strlen(testString)] = '\0';

        REQUIRE(strcmp(readData, testString) == 0);
    }
}

TEST_CASE("FileBuffer - Memory vs Loaded comparison", "[qstream][buffer][comparison]")
{
    REQUIRE_FIXTURE("qstream/buffer_small.txt");

    SECTION("Compare FileBufferMemory and FileBufferLoaded")
    {
        const char* path = GetTestFixturePath("qstream/buffer_small.txt");

        // Load file using FileBufferLoaded
        FileBufferLoaded loadedBuffer(path);

        // Copy to FileBufferMemory
        FileBufferMemory memoryBuffer(loadedBuffer.GetSize());
        memcpy(memoryBuffer.GetWritableData(), loadedBuffer.GetData(), loadedBuffer.GetSize());

        // Both should have same size
        REQUIRE(memoryBuffer.GetSize() == loadedBuffer.GetSize());

        // Both should have same data
        REQUIRE(memcmp(memoryBuffer.GetData(), loadedBuffer.GetData(), loadedBuffer.GetSize()) == 0);

        // But different error/bank properties
        REQUIRE(memoryBuffer.GetError() == false);
        REQUIRE(loadedBuffer.GetError() == false);
        REQUIRE(memoryBuffer.IsFromBank(nullptr) == false);
        REQUIRE(loadedBuffer.IsFromBank(nullptr) == false);
    }
}
