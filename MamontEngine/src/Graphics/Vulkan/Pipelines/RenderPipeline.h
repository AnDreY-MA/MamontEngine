#pragma once 

#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"

namespace MamontEngine
{
    struct GPUSceneData;

    class RenderPipeline
    {
    public:
        RenderPipeline(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, const std::pair<VkFormat, VkFormat> inImageFormats);
        
        ~RenderPipeline() = default;

        void Destroy(VkDevice inDevice);

        std::shared_ptr<PipelineData> OpaquePipeline;
        std::shared_ptr<PipelineData> TransparentPipeline;

        VkDescriptorSetLayout Layout{VK_NULL_HANDLE};
    };
}