#pragma once

namespace MamontEngine::VkPipelines
{
    bool LoadShaderModule(const char *inFilePath, VkDevice inDevice, VkShaderModule *outShaderModule);

    class PipelineBuilder
    {

    public:
        PipelineBuilder()
        {
            Clear();
        }

        void SetLayout(VkPipelineLayout inLayout);

        void SetShaders(VkShaderModule inVertexShader, VkShaderModule inFragmentShader);
        void SetInputTopology(VkPrimitiveTopology inTopology);
        void SetVertexInput(VkPipelineVertexInputStateCreateInfo inInfo);
        void SetPolygonMode(VkPolygonMode inMode);
        void SetCullMode(VkCullModeFlags inCullMode, VkFrontFace inFrontFace);
        void SetMultisamplingNone();
        void DisableBlending();
        void EnableBlendingAdditive();
        void EnableBlendingAlphablend();

        void SetColorAttachmentFormat(VkFormat inFormat);
        void SetDepthFormat(VkFormat inFormat);
        void EnableDepthTest(const bool inDepthWriteEnable, VkCompareOp inOp);
        void EnableDepthClamp(VkBool32 inValue);
        void DisableDepthtest();
        void Clear();

        VkPipeline BuildPipline(VkDevice inDevice, uint32_t attachmentCount = 1);


    private:
        std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;

        VkPipelineInputAssemblyStateCreateInfo m_InputAssembly;
        VkPipelineRasterizationStateCreateInfo m_Rasterizer;
        VkPipelineColorBlendAttachmentState    m_ColorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo   m_Multisampling;
        VkPipelineDepthStencilStateCreateInfo  m_DepthStencil;
        VkPipelineRenderingCreateInfo          m_RenderInfo;
        VkFormat                               m_ColorAttachmentformat;
        VkPipelineLayout                       m_PipelineLayout;
        VkPipelineVertexInputStateCreateInfo   m_VertexInput;

    };
}