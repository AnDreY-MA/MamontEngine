#pragma once

#include "Graphics/Resources/Texture.h"
#include "Graphics/Resources/Materials/MaterialData.h"

namespace MamontEngine
{
    enum class EMaterialPass : uint8_t
    {
        MAIN_COLOR = 0,
        TRANSPARENT,
        OTHER
    };

    struct Material : public NonCopyable
    {
        Material() = default;
        ~Material();

        Material(Material &&other);

        Material &operator=(Material &&other);

        VkDescriptorSet               MaterialSet{VK_NULL_HANDLE};
        EMaterialPass                 PassType{EMaterialPass::MAIN_COLOR};

        std::string Name{std::string("Empty Material")};

        struct MaterialResources
        {
            std::shared_ptr<Texture> ColorTexture;
            std::shared_ptr<Texture> MetalRoughTexture;
            std::shared_ptr<Texture> NormalTexture;
            std::shared_ptr<Texture> EmissiveTexture;
            std::shared_ptr<Texture> OcclusionTexture;
        } Resources;

        struct MaterialConstants
        {
            glm::vec4 ColorFactors = glm::vec4(1.0f);
            float MetalicFactor{0.f};
            float RoughFactor{0.f};

            float _pad0{0.f};
            float _pad1{0.f};

            int HasNormaMap{0};
        } Constants;

        uint32_t Index{0};
        size_t BufferOffset{0};

        bool IsDity{true};

        void Swap(Material &other);
    };
} // namespace MamontEngine

