#include <catch2/catch_test_macros.hpp>
#include <Poseidon/Asset/Formats/Common/FormatDetector.hpp>
#include "test_fixtures.hpp"
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

using namespace Poseidon::Asset::Formats;

TEST_CASE("FormatDetector: MLOD files", "[format][detector][mlod]")
{
    SECTION("Detect complex_vehicle_mlod.p3d (MLOD)")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("mlod/complex_vehicle_mlod.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.GetMLODMajorVersion() == 1);
        REQUIRE(info.GetMLODMinorVersion() == 1);
        REQUIRE(info.GetVersionString() == "1.1");
        REQUIRE(info.isSupported == true);
        REQUIRE(info.errorMessage.empty());
    }

    SECTION("Detect camera_path_a.p3d (MLOD)")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("mlod/camera_path_a.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.version == 0x0101); // Version 1.1
        REQUIRE(info.isSupported == true);
    }

    SECTION("Detect camera_path_b.p3d (MLOD)")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("mlod/camera_path_b.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.isSupported == true);
    }

    SECTION("Detect camera_path_c.p3d (MLOD)")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("mlod/camera_path_c.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.isSupported == true);
    }
}

TEST_CASE("FormatDetector: generated P3D files", "[format][detector][p3d]")
{
    SECTION("Detect empty_shape.p3d")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("p3d/empty_shape.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.isSupported == true);
    }

    SECTION("Detect proxy_structure.p3d")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("p3d/proxy_structure.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.isSupported == true);
    }

    SECTION("Detect sky_plane.p3d")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("p3d/sky_plane.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.isSupported == true);
    }

    SECTION("Detect complex_vehicle.p3d")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(GET_FIXTURE("p3d/complex_vehicle.p3d"));

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.isSupported == true);
    }
}

TEST_CASE("FormatDetector: Error handling", "[format][detector][errors]")
{
    SECTION("Non-existent file")
    {
        FormatInfo info = P3DFormatDetector::DetectFormat("nonexistent.p3d");

        REQUIRE(info.isSupported == false);
        REQUIRE(info.errorMessage.find("Cannot open") != std::string::npos);
    }

    SECTION("Empty file")
    {
        std::stringstream stream;
        stream.write("", 0);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.isSupported == false);
        REQUIRE(info.errorMessage.find("too small") != std::string::npos);
    }

    SECTION("File too small (< 4 bytes)")
    {
        std::stringstream stream;
        stream.write("ABC", 3);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.isSupported == false);
        REQUIRE(info.errorMessage == "File too small (< 4 bytes)");
    }

    SECTION("Unknown signature")
    {
        std::stringstream stream;
        stream.write("UNKN", 4); // Unknown signature
        uint32_t version = 1;
        stream.write(reinterpret_cast<const char*>(&version), 4);
        stream.seekg(0);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.signature == "UNKN");
        REQUIRE(info.isSupported == false);
        REQUIRE(info.errorMessage.find("Unknown format") != std::string::npos);
    }

    SECTION("Unsupported MLOD version (2.5)")
    {
        std::stringstream stream;
        stream.write("MLOD", 4);
        uint32_t version = 0x0205; // Version 2.5
        stream.write(reinterpret_cast<const char*>(&version), 4);
        stream.seekg(0);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.signature == "MLOD");
        REQUIRE(info.GetMLODMajorVersion() == 2);
        REQUIRE(info.GetMLODMinorVersion() == 5);
        REQUIRE(info.isSupported == false);
        REQUIRE(info.errorMessage.find("Unsupported MLOD") != std::string::npos);
        REQUIRE(info.errorMessage.find("2.5") != std::string::npos);
    }

    SECTION("Unsupported ODOL version (28)")
    {
        std::stringstream stream;
        stream.write("ODOL", 4);
        uint32_t version = 28; // Arma 1 version (not yet supported)
        stream.write(reinterpret_cast<const char*>(&version), 4);
        stream.seekg(0);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.signature == "ODOL");
        REQUIRE(info.version == 28);
        REQUIRE(info.isSupported == false);
        REQUIRE(info.errorMessage.find("Unsupported ODOL") != std::string::npos);
        REQUIRE(info.errorMessage.find("28") != std::string::npos);
    }
}

TEST_CASE("FormatDetector: Stream preservation", "[format][detector][stream]")
{
    SECTION("Stream position restored after detection")
    {
        std::stringstream stream;
        stream.write("MLOD", 4);
        uint32_t version = 0x0100;
        stream.write(reinterpret_cast<const char*>(&version), 4);
        uint32_t lodCount = 12;
        stream.write(reinterpret_cast<const char*>(&lodCount), 4);

        stream.seekg(0);
        std::streampos startPos = stream.tellg();

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        std::streampos endPos = stream.tellg();
        REQUIRE(startPos == endPos); // Stream position unchanged
        REQUIRE(info.isSupported == true);
    }
}

TEST_CASE("FormatDetector: Version string formatting", "[format][detector][version]")
{
    SECTION("MLOD version string")
    {
        std::stringstream stream;
        stream.write("MLOD", 4);
        uint32_t version = 0x0304; // Version 3.4
        stream.write(reinterpret_cast<const char*>(&version), 4);
        stream.seekg(0);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.GetVersionString() == "3.4");
    }

    SECTION("ODOL version string")
    {
        std::stringstream stream;
        stream.write("ODOL", 4);
        uint32_t version = 40;
        stream.write(reinterpret_cast<const char*>(&version), 4);
        stream.seekg(0);

        FormatInfo info = P3DFormatDetector::DetectFormat(stream);

        REQUIRE(info.GetVersionString() == "40");
    }
}

TEST_CASE("FormatDetector: All fixture files cataloged", "[format][detector][fixtures]")
{
    // This test documents all fixture files
    std::vector<std::string> mlodFiles = {GET_FIXTURE("mlod/camera_path_a.p3d"), GET_FIXTURE("mlod/camera_path_b.p3d"),
                                          GET_FIXTURE("mlod/camera_path_c.p3d"),
                                          GET_FIXTURE("mlod/complex_vehicle_mlod.p3d")};

    std::vector<std::string> p3dFiles = {GET_FIXTURE("p3d/empty_shape.p3d"), GET_FIXTURE("p3d/proxy_structure.p3d"),
                                         GET_FIXTURE("p3d/sky_plane.p3d"), GET_FIXTURE("p3d/complex_vehicle.p3d")};

    int mlodCount = 0;
    int p3dMlodCount = 0;

    for (const auto& file : mlodFiles)
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(file);
        if (info.signature == "MLOD")
            mlodCount++;
        REQUIRE(info.isSupported);
    }

    for (const auto& file : p3dFiles)
    {
        FormatInfo info = P3DFormatDetector::DetectFormat(file);
        if (info.signature == "MLOD")
            p3dMlodCount++;
        REQUIRE(info.isSupported);
    }

    REQUIRE(mlodCount == 4);
    REQUIRE(p3dMlodCount == 4);
}
