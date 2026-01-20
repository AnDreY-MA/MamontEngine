#pragma once

#include "Graphics/RenderData.h"

namespace MamontEngine
{
    class RenderPass
    {
    public:
        RenderPass()          = default;
        virtual ~RenderPass() = default;

        virtual void Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj) {};
    };
} // namespace MamontEngine
