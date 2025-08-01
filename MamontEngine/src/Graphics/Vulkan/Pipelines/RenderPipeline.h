#pragma once 
#include <Graphics/Vulkan/VkMaterial.h>
#include "Core/RenderData.h"

namespace MamontEngine
{
    struct GPUSceneData;

    class RenderPipeline
    {
    public:
        RenderPipeline() = default;
        ~RenderPipeline() = default;

        void Init(VkDevice inDevice, 
            VkDescriptorSetLayout inDescriptorLayout, 
            std::pair<VkFormat, VkFormat> inImageFormats /*Draw and Depth*/, 
            VkRenderPass inRenderPass);

        void Draw(VkCommandBuffer inCmd, const VkDescriptorSet& globalDescriptor, 
             GPUSceneData &inSceneData, const VkExtent2D& inDrawExtent);

        MaterialPipeline OpaquePipeline;
        MaterialPipeline TransparentPipeline;

        VkDescriptorSetLayout Layout;

        DrawContext MainDrawContext;

    };
}