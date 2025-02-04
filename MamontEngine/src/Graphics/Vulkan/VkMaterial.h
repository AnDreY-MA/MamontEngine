#pragma once 

namespace MamontEngine
{
    enum class EMaterialPass : uint8_t
    {
        MAIN_COLOR,
        TRANSPARENT,
        OTHER
    };

    struct MaterialPipeline
    {
        VkPipeline       Pipeline;
        VkPipelineLayout Layout;
    };

    struct MaterialInstance
    {
        MaterialPipeline *Pipeline;
        VkDescriptorSet   MaterialSet;
        EMaterialPass     PassType;
    };

    struct GLTFMaterial
    {
        MaterialInstance Data;
    };
}