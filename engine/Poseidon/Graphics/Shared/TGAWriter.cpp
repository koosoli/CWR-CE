#include <Poseidon/Graphics/Shared/TGAWriter.hpp>
#include <stb_image_write.h>

namespace Poseidon
{
namespace TGAWriter
{

bool WriteTGA(const char* path, int w, int h, int channels, const uint8_t* pixels)
{
    if (!path || !pixels || w <= 0 || h <= 0 || channels < 1 || channels > 4)
        return false;
    return stbi_write_tga(path, w, h, channels, pixels) != 0;
}

} // namespace TGAWriter

} // namespace Poseidon
