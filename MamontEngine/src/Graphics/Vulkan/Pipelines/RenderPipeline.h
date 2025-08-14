#pragma once 
#include <Graphics/Vulkan/VkMaterial.h>
#include "Core/RenderData.h"

namespace MamontEngine
{
    struct GPUSceneData;

    class RenderPipeline
    {
    public:
        RenderPipeline(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, const std::pair<VkFormat, VkFormat> inImageFormats);
        
        ~RenderPipeline() = default;

        void Draw(VkCommandBuffer inCmd, const VkDescriptorSet& globalDescriptor, 
             GPUSceneData &inSceneData, const VkExtent2D& inDrawExtent);

        MaterialPipeline OpaquePipeline;
        MaterialPipeline TransparentPipeline;

        VkDescriptorSetLayout Layout;

        DrawContext MainDrawContext;

    };
}