#pragma once

#include "Graphics/Resources/Texture.h"
#include "Graphics/Resources/Materials/MaterialData.h"

namespace MamontEngine
{
    enum class EMaterialPass : uint8_t
    {
        MAIN_COLOR = 0,
        Transparent,
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

        enum MaterialFlags : uint32_t
        {
            MaterialFlags_HasBaseColorTexture = 0,
            MaterialFlags_HasRoughMetalTexture = 1 << 0,
            MaterialFlags_HasNormalTexture = 1 << 1,
            MaterialFlags_HasEmissiveTexture = 1 << 2,
            MaterialFlags_HasOcclusionTexture = 1 << 4
        };

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

            int HasBaseTexture{1};
            int HasMetalRough{0};
            int HasNormaMap{0};
            int HasEmmisive{0};
            int HasOcclusion{0};

            float _pad2[3];
        } Constants;

        uint32_t Index{0};

        bool IsDirty{true};

        void Swap(Material &other);

    private:
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {
            //ar(cereal::make_nvp("DescriptorSet", MaterialSet));
            ar(cereal::make_nvp("MaterialPass", PassType));
        }
    };

} // namespace MamontEngine