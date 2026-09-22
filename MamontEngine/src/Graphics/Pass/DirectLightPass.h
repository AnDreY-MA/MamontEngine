#pragma once

#include "Graphics/Pass/RenderPass.h"

namespace MamontEngine
{
    class Camera;

	class DirectLightPass : public RenderPass
	{
    public:
        DirectLightPass(VkFormat inCascadeDepthFormat, VkImage inCascadeImage);

        ~DirectLightPass();

        virtual void Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj) override;

		void UpdateCascade(const Camera* inCamera, const glm::vec3& inLightDirection);

        void CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat);

        const ShadowCascades &GetCascades() const
        {
            return m_Cascades;
        }

	private:
        ShadowCascades m_Cascades;

        std::array<VkImageView, CASCADECOUNT> m_CascadeImageViews;

        VkImage m_CascadeImage{VK_NULL_HANDLE};
        VkFormat m_CascadeFormat;
	};
}