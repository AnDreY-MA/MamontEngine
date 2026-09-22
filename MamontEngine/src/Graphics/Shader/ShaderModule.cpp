#include "ShaderModule.h"
#include "Core/Log.h"

namespace MamontEngine
{
    ShaderModule::ShaderModule(VkDevice device, std::string_view path, EShaderStage stage) 
        : m_Device(device)
    {
        const auto compiledShader = CompileShaderFromFile(path, {.Stage = stage, .OptimizationLevel = EShaderOptimizationLevel::Performance});

        if (compiledShader.has_value())
        {
            const auto &spirvData = compiledShader.value().SPIRV;

            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.pNext                    = nullptr;
            createInfo.codeSize                 = spirvData.size() * sizeof(uint32_t);
            createInfo.pCode                    = reinterpret_cast<const uint32_t *>(spirvData.data());

            VK_CHECK(vkCreateShaderModule(m_Device, &createInfo, nullptr, &m_Module));
        }
        else
        {
            Log::Error("Failed compile Shader");
        }
    }

    ShaderModule::~ShaderModule()
    {
        if (m_Module != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device, m_Module, nullptr);
        }
    }

}