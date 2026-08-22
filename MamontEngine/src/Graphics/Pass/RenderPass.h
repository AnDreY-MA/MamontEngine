#pragma once

#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"

namespace MamontEngine
{
    class RenderPass
    {
    public:
        RenderPass()          = default;
        virtual ~RenderPass() = default;

        virtual void Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj) {};

    protected:
        std::unique_ptr<PipelineData> m_Pipeline;
    };
} // namespace MamontEngine
