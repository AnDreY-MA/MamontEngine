#pragma once 

#include "Graphics/Vulkan/Image.h"

namespace MamontEngine
{
    enum class EMaterialPass : uint8_t
    {
        MAIN_COLOR = 0,
        TRANSPARENT,
        OTHER
    };

    struct MaterialPipeline
    {
        VkPipeline       Pipeline{VK_NULL_HANDLE};
        VkPipelineLayout Layout{VK_NULL_HANDLE};
    };

    struct MaterialInstance
    {
        MaterialPipeline *Pipeline{nullptr};
        VkDescriptorSet   MaterialSet{VK_NULL_HANDLE};
        EMaterialPass     PassType;
    };

    struct GLTFMaterial
    {
        MaterialInstance Data;
    };

    struct GLTFMetallic_Roughness
    {
        struct MaterialConstants
        {
            glm::vec4 ColorFactors;
            glm::vec4 Metal_rough_factors;
            glm::vec4 Extra[14];
        };

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