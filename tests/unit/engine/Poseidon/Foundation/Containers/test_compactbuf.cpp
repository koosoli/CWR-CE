// Unit tests for CompactBuffer from PoseidonBase containers

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Containers/CompactBuf.hpp>
#include <cstring>

// CompactBuffer Tests

TEST_CASE("CompactBuffer - Reference counted buffer", "[compactbuf]")
{
    SECTION("Create and delete buffer")
    {
        CompactBuffer<int>* buf = CompactBuffer<int>::New(10);
        REQUIRE(buf != nullptr);
        REQUIRE(buf->Size() == 10);
        REQUIRE(buf->RefCounter() == 0);

        buf->Delete();
    }

    SECTION("Reference counting")
    {
        CompactBuffer<int>* buf = CompactBuffer<int>::New(5);

        REQUIRE(buf->RefCounter() == 0);

        buf->AddRef();
        REQUIRE(buf->RefCounter() == 1);

        buf->AddRef();
        REQUIRE(buf->RefCounter() == 2);

        buf->Release();
        REQUIRE(buf->RefCounter() == 1);

        buf->Release(); // This should delete
        // Don't access buf after this - it's been deleted
    }

    SECTION("Data access")
    {
        CompactBuffer<int>* buf = CompactBuffer<int>::New(5);

        // Write data
        int* data = buf->Data();
        for (int i = 0; i < 5; i++)
        {
            data[i] = i * 10;
        }

        // Read data
        const int* constData = buf->Data();
        REQUIRE(constData[0] == 0);
        REQUIRE(constData[1] == 10);
        REQUIRE(constData[2] == 20);
        REQUIRE(constData[3] == 30);
        REQUIRE(constData[4] == 40);

        buf->Delete();
    }

    SECTION("Copy from data array")
    {
        int source[] = {1, 2, 3, 4, 5};
        CompactBuffer<int>* buf = CompactBuffer<int>::Copy(source, 5);

        REQUIRE(buf->Size() == 5);
        const int* data = buf->Data();
        for (int i = 0; i < 5; i++)
        {
            REQUIRE(data[i] == i + 1);
        }

        buf->Delete();
    }

    SECTION("Copy from another buffer")
    {
        int source[] = {10, 20, 30};
        CompactBuffer<int>* buf1 = CompactBuffer<int>::Copy(source, 3);
        CompactBuffer<int>* buf2 = CompactBuffer<int>::Copy(buf1);

        REQUIRE(buf2->Size() == 3);
        const int* data = buf2->Data();
        REQUIRE(data[0] == 10);
        REQUIRE(data[1] == 20);
        REQUIRE(data[2] == 30);

        buf1->Delete();
        buf2->Delete();
    }

    SECTION("Partial copy")
    {
        int source[] = {1, 2, 3, 4, 5};
        CompactBuffer<int>* buf1 = CompactBuffer<int>::Copy(source, 5);

        // Copy only first 3 elements
        CompactBuffer<int>* buf2 = CompactBuffer<int>::Copy(buf1, 3);

        REQUIRE(buf2->Size() == 3);
        const int* data = buf2->Data();
        REQUIRE(data[0] == 1);
        REQUIRE(data[1] == 2);
        REQUIRE(data[2] == 3);

        buf1->Delete();
        buf2->Delete();
    }
}

TEST_CASE("CompactBuffer - Different POD types", "[compactbuf]")
{
    SECTION("Float buffer")
    {
        CompactBuffer<float>* buf = CompactBuffer<float>::New(3);

        float* data = buf->Data();
        data[0] = 1.5f;
        data[1] = 2.5f;
        data[2] = 3.5f;

        REQUIRE(buf->Data()[0] == 1.5f);
        REQUIRE(buf->Data()[1] == 2.5f);
        REQUIRE(buf->Data()[2] == 3.5f);

        buf->Delete();
    }

    SECTION("Char buffer")
    {
        CompactBuffer<char>* buf = CompactBuffer<char>::New(6);

        char* data = buf->Data();
        data[0] = 'H';
        data[1] = 'e';
        data[2] = 'l';
        data[3] = 'l';
        data[4] = 'o';
        data[5] = '\0';

        REQUIRE(strcmp(buf->Data(), "Hello") == 0);

        buf->Delete();
    }
}

// CompactString Tests (typedef of CompactBuffer<char>)

TEST_CASE("CompactString - Char buffer specialization", "[compactbuf]")
{
    SECTION("Create string buffer")
    {
        CompactString* str = CompactString::New(10);
        REQUIRE(str->Size() == 10);

        // Write string data
        char* data = str->Data();
        strcpy(data, "Hello");

        REQUIRE(strcmp(str->Data(), "Hello") == 0);

        str->Delete();
    }

    SECTION("Copy string from C string")
    {
        const char* source = "Test String";
        int len = (int)strlen(source) + 1; // Include null terminator
        CompactString* str = CompactString::Copy(source, len);

        REQUIRE(str->Size() == len);
        REQUIRE(strcmp(str->Data(), "Test String") == 0);

        str->Delete();
    }

    SECTION("Copy string from buffer")
    {
        const char* source = "Original";
        int len = (int)strlen(source) + 1;
        CompactString* str1 = CompactString::Copy(source, len);
        CompactString* str2 = CompactString::Copy(str1);

        REQUIRE(strcmp(str1->Data(), str2->Data()) == 0);

        str1->Delete();
        str2->Delete();
    }

    SECTION("Partial string copy")
    {
        const char* source = "LongString";
        int fullLen = (int)strlen(source) + 1;
        CompactString* str1 = CompactString::Copy(source, fullLen);

        // Copy only first 5 characters
        CompactString* str2 = CompactString::Copy(str1, 5);

        REQUIRE(str2->Size() == 5);
        REQUIRE(strncmp(str2->Data(), "LongS", 5) == 0);

        str1->Delete();
        str2->Delete();
    }
}

TEST_CASE("CompactBuffer - Reference counting lifecycle", "[compactbuf]")
{
    SECTION("Multiple references")
    {
        CompactBuffer<int>* buf = CompactBuffer<int>::New(3);

        // Simulate multiple owners
        buf->AddRef(); // Owner 1
        buf->AddRef(); // Owner 2
        buf->AddRef(); // Owner 3

        REQUIRE(buf->RefCounter() == 3);

        // Owners release one by one
        buf->Release(); // Owner 1 done
        REQUIRE(buf->RefCounter() == 2);

        buf->Release(); // Owner 2 done
        REQUIRE(buf->RefCounter() == 1);

        buf->Release(); // Owner 3 done - buffer deleted here
        // Don't access buf after this
    }

    SECTION("Zero references on creation")
    {
        CompactBuffer<int>* buf = CompactBuffer<int>::New(1);
        REQUIRE(buf->RefCounter() == 0);

        // If nobody takes ownership, manual delete needed
        buf->Delete();
    }
}
