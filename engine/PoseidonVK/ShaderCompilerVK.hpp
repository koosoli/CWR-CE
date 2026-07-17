#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

namespace Poseidon::vk
{
// Runtime compilation is deliberately centralized for embedded GLSL passes.
// Source remains embedded by CMake, so installed builds have no shader-file
// lookup or working-directory dependency.
bool CompileEmbeddedGlsl(const char* source, VkShaderStageFlagBits stage, std::vector<std::uint32_t>& spirv,
                         std::string& error);
bool CreateEmbeddedShaderModule(VkDevice device, const std::vector<std::uint32_t>& spirv, VkShaderModule& module);
} // namespace Poseidon::vk
