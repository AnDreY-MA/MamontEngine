#pragma once

#include "RenderPass.h"

namespace MamontEngine
{
	class PickPass : public RenderPass
	{
    public:
        explicit PickPass();
        ~PickPass();

        void CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat);
    private:
        std::unique_ptr<PipelineData> m_OutlinePipeline;

        VkDescriptorSet m_IdDescriptorSet{VK_NULL_HANDLE};
        VkDescriptorSetLayout m_IdDescriptorSetLayout{VK_NULL_HANDLE};

	};
}