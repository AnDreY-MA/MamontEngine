#pragma once

#include "Core/FrameData.h"


namespace MamontEngine
{
    struct ComputePushConstants
    {
        glm::vec4 Data1;
        glm::vec4 Data2;
        glm::vec4 Data3;
        glm::vec4 Data4;
    };

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
        
        SkyPipeline(VkDevice inDevice, const VkDescriptorSetLayout *inDrawImageDescriptorLayout);

        ~SkyPipeline() = default;

        void Draw(VkCommandBuffer inCmd, const VkDescriptorSet *inDescriptorSet);

		void Destroy(VkDevice inDevice);

	private:
        VkPipelineLayout m_GradientPipelineLayout;
        ComputeEffect    m_Effect;

	};
}