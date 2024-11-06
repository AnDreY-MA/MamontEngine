#pragma once

#include "Core/RenderData.h"

namespace MamontEngine
{
    struct ComputeEffect
    {
        const char      *Name;
        ::VkPipeline     Pipeline;
        VkPipelineLayout Layout;

        ComputePushConstants Data;
    };

	class SkyPipeline
	{
    public:
        void Init(class MDevice &inDevice, VkDescriptorSetLayout *inDescriptorSetLayout, DeletionQueue &inDeletQueue);

	private:
        VkPipelineLayout m_PipelineLayout;
        VkPipeline       m_Pipeline;
        std::vector<ComputeEffect> m_BackgroundEffects;
        int                        m_CurrentBackgroundEffect{1};

        friend class MEngine;
	};
}
