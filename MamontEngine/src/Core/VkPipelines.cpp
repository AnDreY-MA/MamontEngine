#include "VkPipelines.h"
#include <fstream>
#include "VkInitializers.h"

namespace MamontEngine::VkPipeline
{
    bool LoadShaderModule(const char *inFilePath, VkDevice inDevice, VkShaderModule *outShaderModule)
    {
        std::ifstream file(inFilePath, std::ios::ate | std::ios::binary);
        if (!file.is_open())
            return false;

        size_t fileSize = (size_t)file.tellg();
        
        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read((char *)buffer.data(), fileSize);
        file.close();

        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext                    = nullptr;
        createInfo.codeSize                 = buffer.size() * sizeof(uint32_t);
        createInfo.pCode                    = reinterpret_cast<const uint32_t *>(buffer.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(inDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            fmt::println("Create Shader - no success");
            return false;
        }
        *outShaderModule = shaderModule;

        return true;
    }
} // namespace MamontEngine::VkPipeline
