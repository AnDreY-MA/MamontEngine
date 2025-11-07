#pragma once 

#include "Graphics/Vulkan/Image.h"
#include "Graphics/Resources/Texture.h"

namespace MamontEngine
{
    struct PipelineData;

    enum class EMaterialPass : uint8_t
    {
        MAIN_COLOR = 0,
        TRANSPARENT,
        OTHER
    };

    struct Material
    {
        std::shared_ptr<PipelineData> Pipeline;
        VkDescriptorSet               MaterialSet{VK_NULL_HANDLE};
        EMaterialPass PassType{EMaterialPass::MAIN_COLOR};

        std::string Name{std::string("Empty Material")};

        struct MaterialResources
        {
            Texture   ColorTexture;
            Texture   MetalRoughTexture;
            VkBuffer  DataBuffer{VK_NULL_HANDLE};
            uint32_t  DataBufferOffset{0};
        } Resources;

        struct MaterialConstants
        {
            MaterialConstants() = default;

            MaterialConstants(const glm::vec4 &inColorFactor, const float inMetalicFactor, const float inRoughFactor)
                : ColorFactors(inColorFactor), MetalicFactor(inMetalicFactor), RoughFactor(inRoughFactor)
            {
            }
            glm::vec4 ColorFactors = glm::vec4(1.0f);

            float MetalicFactor{0.f};
            float RoughFactor{0.f};
        } Constants;

        bool IsDity{true};

    };

}