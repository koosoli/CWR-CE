#pragma once

#include <cstdint>
#include <istream>
#include <string>

namespace Poseidon::Asset::Formats
{

struct FormatInfo
{
    std::string signature;   // "MLOD", "ODOL", etc.
    uint32_t    version    = 0;
    std::string extension; // File extension (.p3d)
    bool        isSupported  = false;
    std::string errorMessage;

    // MLOD version is encoded as ((major << 8) | minor)
    uint8_t GetMLODMajorVersion() const { return (version >> 8) & 0xFF; }
    uint8_t GetMLODMinorVersion() const { return version & 0xFF; }

    std::string GetVersionString() const
    {
        if (signature == "MLOD")
            return std::to_string(GetMLODMajorVersion()) + "." + std::to_string(GetMLODMinorVersion());
        return std::to_string(version);
    }
};

class P3DFormatDetector
{
  public:
    static FormatInfo DetectFormat(const std::string& filePath);
    static FormatInfo DetectFormat(std::istream& stream, const std::string& extension = "");

  private:
    static bool        IsSupportedMLODVersion(uint32_t version);
    static bool        IsSupportedODOLVersion(uint32_t version);
    static std::string GetFileExtension(const std::string& path);
};

} // namespace Poseidon::Asset::Formats
