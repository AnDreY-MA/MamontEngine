#pragma once

#include "Graphics/Pass/RenderPass.h"
#include "Graphics/Frustum.h"

namespace MamontEngine
{
    class Camera;

	class CascadeShadowPass : public RenderPass
	{
    public:
        CascadeShadowPass(VkFormat inCascadeDepthFormat, VkImage inCascadeImage);

        ~CascadeShadowPass();

        virtual void Render(VkCommandBuffer                            cmd,
                            std::pair<VkDescriptorSet, VkDescriptorSet> globalDescriptor,
                            const DrawContext                         &inDrawContext,
                            const glm::mat4                           &viewproj) override;

		void UpdateCascade(const Camera* inCamera, const glm::vec3& inLightDirection);

        void CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat);

        const std::array<Cascade, CASCADECOUNT> &GetCascades() const
        {
            return m_Cascade;
        }

	private:
        //ShadowCascades m_Cascades;

        std::array<Cascade, CASCADECOUNT> m_Cascade;

        std::array<VkImageView, CASCADECOUNT> m_CascadeImageViews;
        std::array<FrustumPlane, CASCADECOUNT> m_CascadeFrustumPlanes;

        VkImage m_CascadeImage{VK_NULL_HANDLE};
        VkFormat m_CascadeFormat;
	};
}