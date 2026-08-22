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

        const Cascade& GetCascade(size_t inIndex) const 
        {
            return Cascades.at(inIndex);
        }

        const std::array<Cascade, CASCADECOUNT>& GetCascades() const
        {
            return Cascades;
        }


	private:
        std::array<Cascade, CASCADECOUNT> Cascades;

        VkImage m_CascadeImage{VK_NULL_HANDLE};

        AllocatedBuffer                            m_Buffer;
        std::array<AllocatedBuffer, FRAME_OVERLAP> m_StagingBuffers;
	};
}