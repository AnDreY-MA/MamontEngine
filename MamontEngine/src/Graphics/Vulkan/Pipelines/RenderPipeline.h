#pragma once 

#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"

namespace MamontEngine
{
    struct GPUSceneData;

    class RenderPipeline
    {
    public:
        RenderPipeline(VkDevice inDevice, std::span<const VkDescriptorSetLayout> inDescriptorLayouts, const std::pair<VkFormat, VkFormat> inImageFormats);
        
        ~RenderPipeline();

        std::shared_ptr<PipelineData> OpaquePipeline;
        std::shared_ptr<PipelineData> TransparentPipeline;

        std::shared_ptr<PipelineData> SkyboxPipline;

        std::shared_ptr<PipelineData> DebugDrawPipeline;
    };
}