#include <PoseidonVK/ShaderCompilerVK.hpp>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

namespace Poseidon::vk
{
namespace
{
EShLanguage ToGlslangStage(VkShaderStageFlagBits stage)
{
    switch (stage)
    {
        case VK_SHADER_STAGE_VERTEX_BIT: return EShLangVertex;
        case VK_SHADER_STAGE_FRAGMENT_BIT: return EShLangFragment;
        case VK_SHADER_STAGE_COMPUTE_BIT: return EShLangCompute;
        default: return EShLangCount;
    }
}

void EnsureGlslangInitialized()
{
    static const bool initialized = []
    {
        glslang::InitializeProcess();
        return true;
    }();
    (void)initialized;
}
} // namespace

bool CompileEmbeddedGlsl(const char* source, VkShaderStageFlagBits stage, std::vector<std::uint32_t>& spirv,
                         std::string& error)
{
    spirv.clear();
    error.clear();
    const EShLanguage glslangStage = ToGlslangStage(stage);
    if (source == nullptr || glslangStage == EShLangCount)
    {
        error = "unsupported embedded GLSL stage or null source";
        return false;
    }

    EnsureGlslangInitialized();
    glslang::TShader shader(glslangStage);
    shader.setStrings(&source, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, glslangStage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);
    const EShMessages messages = static_cast<EShMessages>(EShMsgVulkanRules | EShMsgSpvRules);
    if (!shader.parse(GetDefaultResources(), 450, false, messages))
    {
        error = shader.getInfoLog();
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(messages))
    {
        error = program.getInfoLog();
        return false;
    }
    glslang::GlslangToSpv(*program.getIntermediate(glslangStage), spirv);
    if (spirv.empty())
    {
        error = "SPIR-V output is empty";
        return false;
    }
    return true;
}

bool CreateEmbeddedShaderModule(VkDevice device, const std::vector<std::uint32_t>& spirv, VkShaderModule& module)
{
    module = VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE || spirv.empty())
        return false;
    VkShaderModuleCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    info.codeSize = spirv.size() * sizeof(std::uint32_t);
    info.pCode = spirv.data();
    return vkCreateShaderModule(device, &info, nullptr, &module) == VK_SUCCESS;
}
} // namespace Poseidon::vk
