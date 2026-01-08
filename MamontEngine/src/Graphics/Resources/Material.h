#pragma once

#include "Graphics/Resources/Texture.h"
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

    struct Material
    {
        std::shared_ptr<PipelineData> Pipeline;
        VkDescriptorSet               MaterialSet{VK_NULL_HANDLE};
        EMaterialPass                 PassType{EMaterialPass::MAIN_COLOR};

        std::string Name{std::string("Empty Material")};

        struct MaterialResources
        {
            Texture ColorTexture;
            Texture MetalRoughTexture;
            Texture NormalTexture;
            Texture EmissiveTexture;
            Texture OcclusionTexture;
        } Resources;

        struct MaterialConstants
        {
            glm::vec4 ColorFactors = glm::vec4(1.0f);

            float MetalicFactor{0.f};
            float RoughFactor{0.f};

            float _pad0{0.f};
            float _pad1{0.f};
        } Constants;

        size_t BufferOffset{0};

        bool IsDity{true};
    };
} // namespace MamontEngine

