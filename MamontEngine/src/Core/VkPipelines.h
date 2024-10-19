#pragma once

namespace MamontEngine::VkPipeline
{
    bool LoadShaderModule(const char *inFilePath, VkDevice inDevice, VkShaderModule *outShaderModule);
}