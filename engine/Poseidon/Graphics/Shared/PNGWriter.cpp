#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <Poseidon/Graphics/Shared/PNGWriter.hpp>

namespace Poseidon
{
namespace PNGWriter
{

bool WritePNG(const char* path, int w, int h, int channels, const uint8_t* pixels)
{
    if (!path || !pixels || w <= 0 || h <= 0 || channels < 1 || channels > 4)
        return false;
    return stbi_write_png(path, w, h, channels, pixels, w * channels) != 0;
}

} // namespace PNGWriter

} // namespace Poseidon
