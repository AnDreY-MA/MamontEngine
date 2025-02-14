#include "SkyPipeline.h"

#include "Core/VkPipelines.h"


namespace MamontEngine
{
    void SkyPipeline::Init(VkDevice inDevice, const VkDescriptorSetLayout *inDrawImageDescriptorLayout, DeletionQueue &inDeletionQueue)
    {
        VkPipelineLayoutCreateInfo computeLayout{};
        computeLayout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computeLayout.pNext          = nullptr;
        computeLayout.pSetLayouts    = inDrawImageDescriptorLayout;
        computeLayout.setLayoutCount = 1;

        VkPushConstantRange pushConstant{};
        pushConstant.offset     = 0;
        pushConstant.size       = sizeof(ComputePushConstants);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pPushConstantRanges    = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(inDevice, &computeLayout, nullptr, &m_GradientPipelineLayout));

        const std::string shaderPath = std::string(PROJECT_ROOT_DIR) + "/MamontEngine/src/Shaders/gradient_color.comp.spv";
        VkShaderModule    gradientShader;
        if (!VkPipelines::LoadShaderModule(shaderPath.c_str(), inDevice, &gradientShader))
        {
            fmt::print("Error when building the Gradient shader \n");
        }

        VkShaderModule    skyShader;
        const std::string skyShaderPath = std::string(PROJECT_ROOT_DIR) + "/MamontEngine/src/Shaders/sky.comp.spv";
        if (!VkPipelines::LoadShaderModule(skyShaderPath.c_str(), inDevice, &skyShader))
        {
            fmt::print("Error when building the Sky shader ");
        }

        VkPipelineShaderStageCreateInfo stageinfo{};
        stageinfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageinfo.pNext  = nullptr;
        stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        stageinfo.module = gradientShader;
        stageinfo.pName  = "main";

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.pNext  = nullptr;
        computePipelineCreateInfo.layout = m_GradientPipelineLayout;
        computePipelineCreateInfo.stage  = stageinfo;

        ComputeEffect gradient{};
        gradient.Layout = m_GradientPipelineLayout;
        gradient.Name   = "gradient";
        gradient.Data   = {};

        gradient.Data.Data1 = glm::vec4(1, 0, 0, 1);
        gradient.Data.Data2 = glm::vec4(0, 0, 1, 1);

        VK_CHECK(vkCreateComputePipelines(inDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.Pipeline));

        computePipelineCreateInfo.stage.module = skyShader;

        ComputeEffect sky{};
        sky.Layout     = m_GradientPipelineLayout;
        sky.Name       = "Sky";
        sky.Data       = {};
        sky.Data.Data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

        VK_CHECK(vkCreateComputePipelines(inDevice, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.Pipeline));

        m_Effect = sky;

        vkDestroyShaderModule(inDevice, gradientShader, nullptr);
        vkDestroyShaderModule(inDevice, skyShader, nullptr);

        inDeletionQueue.PushFunction(
                [=]()
                {
                    vkDestroyPipelineLayout(inDevice, m_GradientPipelineLayout, nullptr);
                    vkDestroyPipeline(inDevice, m_Effect.Pipeline, nullptr);
                });
    }

    void SkyPipeline::Draw(VkCommandBuffer inCmd, const VkDescriptorSet *inDescriptorSet)
    {
        vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_Effect.Pipeline);

        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GradientPipelineLayout, 0, 1, inDescriptorSet, 0, nullptr);

        vkCmdPushConstants(inCmd, m_GradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &m_Effect.Data);
    }

    void SkyPipeline::Destroy(VkDevice inDevice)
    {
        vkDestroyPipelineLayout(inDevice, m_GradientPipelineLayout, nullptr);
    }
}