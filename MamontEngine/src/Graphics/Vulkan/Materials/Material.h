#pragma once 

#include "Graphics/Vulkan/Image.h"

namespace MamontEngine
{
    struct PipelineData;

    enum class EMaterialPass : uint8_t
    {
        MAIN_COLOR = 0,
        TRANSPARENT,
        OTHER
    };

    struct GLTFMaterial
    {
        std::shared_ptr<PipelineData> Pipeline;
        VkDescriptorSet               MaterialSet{VK_NULL_HANDLE};
        EMaterialPass                 PassType;

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
    };

    struct GLTFMetallic_Roughness
    {
        struct MaterialResources
        {
            AllocatedImage ColorImage;
            VkSampler      ColorSampler{VK_NULL_HANDLE};
            AllocatedImage MetalRoughImage;
            VkSampler      MetalRoughSampler{VK_NULL_HANDLE};
            VkBuffer       DataBuffer{VK_NULL_HANDLE};
            uint32_t       DataBufferOffset{0};
        };
    };
}