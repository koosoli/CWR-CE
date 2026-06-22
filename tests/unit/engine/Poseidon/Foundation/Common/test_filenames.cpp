// Unit tests for filename manipulation functions from Poseidon/Foundation/Common/Filenames.hpp

#define _CRT_SECURE_NO_WARNINGS // Allow strcpy in test code

#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Foundation/Common/Filenames.hpp>
#include <string.h>

TEST_CASE("GetFilenameExt extracts filename with extension", "[filenames]")
{
    SECTION("Simple path with backslash")
    {
        const char* path = "C:\\folder\\file.txt";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "file.txt") == 0);
    }

    SECTION("Path with multiple backslashes")
    {
        const char* path = "C:\\users\\documents\\test\\myfile.dat";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "myfile.dat") == 0);
    }

    SECTION("Filename without path")
    {
        const char* path = "simple.txt";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "simple.txt") == 0);
    }

    SECTION("Drive letter with colon")
    {
        const char* path = "D:file.bin";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "file.bin") == 0);
    }

    SECTION("Drive letter without backslash")
    {
        const char* path = "E:test.exe";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "test.exe") == 0);
    }

    SECTION("Empty path")
    {
        const char* path = "";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "") == 0);
    }

    SECTION("Path ending with backslash")
    {
        const char* path = "C:\\folder\\";
        const char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "") == 0);
    }

    SECTION("Mutable string version")
    {
        char path[] = "C:\\test\\file.txt";
        char* result = GetFilenameExt(path);
        REQUIRE(strcmp(result, "file.txt") == 0);
        // Verify it returns pointer to same buffer
        REQUIRE(result >= path);
        REQUIRE(result < path + strlen(path) + 1);
    }
}

TEST_CASE("GetFileExt extracts file extension", "[filenames]")
{
    SECTION("Simple extension")
    {
        const char* name = "file.txt";
        const char* result = GetFileExt(name);
        REQUIRE(strcmp(result, ".txt") == 0);
    }

    SECTION("Multiple dots in filename")
    {
        const char* name = "archive.tar.gz";
        const char* result = GetFileExt(name);
        REQUIRE(strcmp(result, ".gz") == 0);
    }

    SECTION("No extension")
    {
        const char* name = "README";
        const char* result = GetFileExt(name);
        REQUIRE(strcmp(result, "") == 0);
        // Points to the null terminator
        REQUIRE(*result == '\0');
    }

    SECTION("Hidden file with extension (Unix style)")
    {
        const char* name = ".gitignore";
        const char* result = GetFileExt(name);
        REQUIRE(strcmp(result, ".gitignore") == 0);
    }

    SECTION("Empty string")
    {
        const char* name = "";
        const char* result = GetFileExt(name);
        REQUIRE(strcmp(result, "") == 0);
    }

    SECTION("Full path with extension")
    {
        const char* name = "C:\\folder\\file.dat";
        const char* result = GetFileExt(name);
        REQUIRE(strcmp(result, ".dat") == 0);
    }

    SECTION("Mutable string version")
    {
        char name[] = "test.cpp";
        char* result = GetFileExt(name);
        REQUIRE(strcmp(result, ".cpp") == 0);
        // Verify it returns pointer to same buffer
        REQUIRE(result >= name);
        REQUIRE(result < name + strlen(name) + 1);
    }
}

TEST_CASE("TerminateBy adds terminating character", "[filenames]")
{
    SECTION("Add backslash to path without it")
    {
        char path[32] = "C:\\folder";
        TerminateBy(path, '\\');
        REQUIRE(strcmp(path, "C:\\folder\\") == 0);
    }

    SECTION("Path already has terminating backslash")
    {
        char path[32] = "C:\\folder\\";
        TerminateBy(path, '\\');
        REQUIRE(strcmp(path, "C:\\folder\\") == 0);
    }

    SECTION("Add semicolon")
    {
        char str[32] = "statement";
        TerminateBy(str, ';');
        REQUIRE(strcmp(str, "statement;") == 0);
    }

    SECTION("Empty string")
    {
        char str[32] = "";
        TerminateBy(str, '/');
        REQUIRE(strcmp(str, "/") == 0);
    }

    SECTION("Single character string")
    {
        char str[32] = "x";
        TerminateBy(str, 'y');
        REQUIRE(strcmp(str, "xy") == 0);
    }

    SECTION("Already terminated with requested character")
    {
        char str[32] = "path/";
        TerminateBy(str, '/');
        REQUIRE(strcmp(str, "path/") == 0);
    }
}

TEST_CASE("GetFilename extracts filename without extension", "[filenames]")
{
    SECTION("Simple filename with extension")
    {
        char dst[32];
        GetFilename(dst, "file.txt");
        REQUIRE(strcmp(dst, "file") == 0);
    }

    SECTION("Full path with extension")
    {
        char dst[32];
        GetFilename(dst, "C:\\folder\\test.dat");
        REQUIRE(strcmp(dst, "test") == 0);
    }

    SECTION("Multiple dots")
    {
        char dst[32];
        GetFilename(dst, "archive.tar.gz");
        REQUIRE(strcmp(dst, "archive") == 0);
    }

    SECTION("No extension")
    {
        char dst[32];
        GetFilename(dst, "README");
        REQUIRE(strcmp(dst, "README") == 0);
    }

    SECTION("With path but no extension")
    {
        char dst[32];
        GetFilename(dst, "C:\\folder\\README");
        REQUIRE(strcmp(dst, "README") == 0);
    }

    SECTION("Path with multiple folders")
    {
        char dst[32];
        GetFilename(dst, "C:\\users\\docs\\project\\main.cpp");
        REQUIRE(strcmp(dst, "main") == 0);
    }

    SECTION("Empty string")
    {
        char dst[32] = "initial";
        GetFilename(dst, "");
        REQUIRE(strcmp(dst, "") == 0);
    }
}

TEST_CASE("IsRelativePath checks if path is relative", "[filenames]")
{
    SECTION("Relative paths return true")
    {
        REQUIRE(IsRelativePath("folder\\file.txt") == true);
        REQUIRE(IsRelativePath("file.txt") == true);
        REQUIRE(IsRelativePath("subfolder\\another\\file.dat") == true);
    }

    SECTION("Absolute paths with backslash return false")
    {
        REQUIRE(IsRelativePath("\\folder\\file.txt") == false);
    }

    SECTION("Absolute paths with forward slash return false")
    {
        REQUIRE(IsRelativePath("/folder/file.txt") == false);
    }

    SECTION("Paths with drive letter return false")
    {
        REQUIRE(IsRelativePath("C:\\folder\\file.txt") == false);
        REQUIRE(IsRelativePath("D:file.dat") == false);
    }

    SECTION("Paths with parent directory traversal return false")
    {
        REQUIRE(IsRelativePath("..\\file.txt") == false);
        REQUIRE(IsRelativePath("folder\\..\\file.txt") == false);
    }

    SECTION("Empty string is relative")
    {
        REQUIRE(IsRelativePath("") == true);
    }

    SECTION("Simple filename is relative")
    {
        REQUIRE(IsRelativePath("file.txt") == true);
    }
}
