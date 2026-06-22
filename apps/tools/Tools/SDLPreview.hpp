#pragma once
#include <SDL3/SDL.h>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace PoseidonTools
{

// Callback for interactive resize: (newW, newH) -> updated pixel data
using ResizeCallback = std::function<std::vector<uint8_t>(int, int)>;

// Display RGBA pixels in an SDL window. Blocks until user closes window.
void DisplayWindowRGBA(const std::string& title, int imgW, int imgH, const uint8_t* pixels);

// Display RGB pixels in an SDL window with optional resize callback.
void DisplayWindowRGB(const std::string& title, int imgW, int imgH, const uint8_t* pixels,
                      ResizeCallback onResize = nullptr);

} // namespace PoseidonTools
