#pragma once

#include "RenderPass.h"
#include <Graphics/RenderData.h>

namespace MamontEngine
{
    class MainPass final : public RenderPass
    {
    public:
        explicit MainPass();
        ~MainPass() = default;

        virtual void Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj) override;

    };
} // namespace MamontEngine
