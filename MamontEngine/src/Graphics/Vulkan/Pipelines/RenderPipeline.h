#pragma once 

#include "Graphics/Vulkan/Materials/Material.h"
#include "Core/RenderData.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"

namespace MamontEngine
{
    struct GPUSceneData;

    class RenderPipeline
    {
    public:
        RenderPipeline(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, const std::pair<VkFormat, VkFormat> inImageFormats);
        
        ~RenderPipeline() = default;

        void Draw(VkCommandBuffer inCmd, const VkDescriptorSet& globalDescriptor, 
             const GPUSceneData &inSceneData, const VkExtent2D& inDrawExtent);

        void Destroy(VkDevice inDevice);

        std::shared_ptr<PipelineData> OpaquePipeline;
        std::shared_ptr<PipelineData> TransparentPipeline;

        VkDescriptorSetLayout Layout{VK_NULL_HANDLE};

        DrawContext MainDrawContext;

    };
}