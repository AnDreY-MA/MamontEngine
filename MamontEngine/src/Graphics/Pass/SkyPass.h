#pragma once

#include "Graphics/Vulkan/Pipelines/PipelineData.h"
#include "Graphics/RenderData.h"
#include "Graphics/Resources/Models/Model.h"

namespace MamontEngine
{
	class SkyPass
	{
    public:
        explicit SkyPass();
        ~SkyPass();
        void CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, std::pair<VkFormat, VkFormat> inImageFormats);

		void Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);

	private:
        std::unique_ptr<PipelineData> m_Pipeline;
        DrawContext                   m_SkyboxContext;
        std::unique_ptr<MeshModel>    m_Skybox;
        std::pair<VkFormat, VkFormat> m_ImageFormats;

	};
}