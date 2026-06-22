#include <Poseidon/Asset/Formats/Common/FormatDetector.hpp>
#include <cstring>
#include <fstream>

namespace Poseidon::Asset::Formats
{

FormatInfo P3DFormatDetector::DetectFormat(const std::string& filePath)
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream)
    {
        FormatInfo info;
        info.isSupported = false;
        info.errorMessage = "Cannot open file: " + filePath;
        info.extension = GetFileExtension(filePath);
        return info;
    }
    return DetectFormat(stream, GetFileExtension(filePath));
}

FormatInfo P3DFormatDetector::DetectFormat(std::istream& stream, const std::string& extension)
{
    FormatInfo info;
    info.extension = extension;

    std::streampos startPos = stream.tellg();

    char signature[5] = {0};
    stream.read(signature, 4);
    if (stream.gcount() < 4)
    {
        info.isSupported = false;
        info.errorMessage = "File too small (< 4 bytes)";
        stream.seekg(startPos);
        return info;
    }
    info.signature = std::string(signature, 4);

    uint32_t version = 0;
    stream.read(reinterpret_cast<char*>(&version), 4);
    if (stream.fail())
    {
        info.isSupported = false;
        info.errorMessage = "Cannot read version field";
        stream.seekg(startPos);
        return info;
    }
    info.version = version;

    stream.seekg(startPos);

    if (info.signature == "MLOD")
    {
        info.isSupported = IsSupportedMLODVersion(version);
        if (!info.isSupported)
            info.errorMessage = "Unsupported MLOD version: " + info.GetVersionString() + " (only 1.0 supported)";
    }
    else if (info.signature == "ODOL")
    {
        info.isSupported = IsSupportedODOLVersion(version);
        if (!info.isSupported)
            info.errorMessage = "Unsupported ODOL version: " + std::to_string(version) + " (only v7 and v8 supported)";
    }
    else
    {
        info.isSupported = false;
        bool hasPrintable = false;
        for (int i = 0; i < 4; ++i)
        {
            if (signature[i] >= 32 && signature[i] <= 126)
            {
                hasPrintable = true;
                break;
            }
        }
        info.errorMessage = hasPrintable ? "Unknown format signature: '" + info.signature + "'"
                                         : "Not a valid P3D file (invalid signature)";
    }

    return info;
}

bool P3DFormatDetector::IsSupportedMLODVersion(uint32_t version)
{
    uint8_t major = (version >> 8) & 0xFF;
    uint8_t minor = version & 0xFF;
    // MLOD 1.0 (0x0100) and 1.1 (0x0101) — both versions present in OFP/CWA files
    return (major == 1 && (minor == 0 || minor == 1));
}

bool P3DFormatDetector::IsSupportedODOLVersion(uint32_t version)
{
    // ODOL v7 and v8 (Arma: Cold War Assault)
    return (version == 7 || version == 8);
}

std::string P3DFormatDetector::GetFileExtension(const std::string& path)
{
    size_t dot = path.find_last_of('.');
    if (dot != std::string::npos && dot < path.length() - 1)
        return path.substr(dot + 1);
    return "";
}

} // namespace Poseidon::Asset::Formats
