#pragma once 

#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"

namespace MamontEngine
{
    struct GPUSceneData;

    class RenderPipeline
    {
    public:
        RenderPipeline(VkDevice inDevice, const std::span<VkDescriptorSetLayout> inDescriptorLayouts, const std::pair<VkFormat, VkFormat> inImageFormats);
        
        ~RenderPipeline();

        void Destroy(VkDevice inDevice);

        std::shared_ptr<PipelineData> OpaquePipeline;
        std::shared_ptr<PipelineData> TransparentPipeline;
    };
}