#pragma once 

namespace MamontEngine
{
	class MeshPipeline
	{
	public:
        void Init(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, VkFormat inDrawImageFormat, VkFormat inDepthImageFormat);
	};
}