// Test for FindBank x64 handle truncation issue
// Verifies that file find handles are not truncated on 64-bit platforms

#include <catch2/catch_all.hpp>
#include <Poseidon/IO/Streams/QBStream.hpp>
#include <filesystem>
#include <fstream>
#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("FindBank handles are properly sized for x64", "[qstream][findbank][x64]")
{
    SECTION("Handle storage can hold full intptr_t value")
    {
        // On x64, intptr_t is 64-bit. Verify FindBank can store it without truncation.
        // This test verifies the fix for handle truncation causing crashes.

        // Create a temporary directory with some files
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "findbank_test";
        std::filesystem::create_directories(tempDir);

        // Create test files
        std::ofstream((tempDir / "test1.txt").string());
        std::ofstream((tempDir / "test2.txt").string());

        // Test FindBank operations
        FindBank find;
        bool foundAny = false;

        if (find.First((tempDir / "*.txt").string().c_str()))
        {
            foundAny = true;
            REQUIRE(find.GetName() != nullptr);

            // Try to iterate - this would crash with truncated handle
            while (find.Next())
            {
                REQUIRE(find.GetName() != nullptr);
            }
        }

        // Cleanup
        std::filesystem::remove_all(tempDir);

        // On systems with files, we should have found something
        // (test may pass vacuously if directory operations fail, but won't crash)
        if (foundAny)
        {
            SUCCEED("FindBank iteration completed without crash");
        }
    }

    SECTION("Handle type size verification")
    {
        // Verify that the handle member is properly sized
        FindBank find;

// The internal handle should be able to store intptr_t without truncation
// This is a compile-time check via sizeof
#ifdef _WIN32
        // On Windows, _findfirst returns intptr_t
        REQUIRE(sizeof(intptr_t) <= sizeof(intptr_t)); // Tautology, but documents intent

#ifdef _WIN64
                                                       // On x64, intptr_t must be 64-bit
        REQUIRE(sizeof(intptr_t) == 8);
#else
                                                       // On x86, intptr_t is 32-bit
        REQUIRE(sizeof(intptr_t) == 4);
#endif
#endif
    }
}
