#pragma once


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

        EMaterialPass                 PassType{EMaterialPass::MAIN_COLOR};

        std::string Name{std::string("Empty Material")};

        enum MaterialFlags : uint32_t
        {
            MaterialFlags_HasBaseColorTexture = 0,
            MaterialFlags_HasMetalRoughTexture = 1u << 0,
            MaterialFlags_HasNormalTexture = 1u << 1,
            MaterialFlags_HasEmissiveTexture = 1u << 2,
            MaterialFlags_HasOcclusionTexture = 1u << 4
        };

        struct MaterialConstants
        {
            glm::vec4 ColorFactors = glm::vec4(1.0f);
            float MetalicFactor{0.f};
            float RoughFactor{0.f};
            uint32_t  Flags{0};

            float _pad0{0.f};
            float _pad1{0.f};

            int BaseTextureIndex{0};
            int MetalRoughTextureIndex{0};
            int NormaTextureIndex{0};
            int EmmisiveTextureIndex{0};
            int OcclusionTexture{0};

            float _pad2[2];
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