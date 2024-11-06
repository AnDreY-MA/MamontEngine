#include "SkyPipeline.h"
#include "MDevice.h"
#include "Core/VkPipelines.h"

namespace MamontEngine
{
    void SkyPipeline::Init(MDevice &inDevice, VkDescriptorSetLayout* inDescriptorSetLayout, DeletionQueue &inDeletQueue)
    {
        const auto                &device = inDevice.GetDevice();
        VkPipelineLayoutCreateInfo computeLayout{};
        computeLayout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computeLayout.pNext          = nullptr;
        computeLayout.pSetLayouts    = inDescriptorSetLayout;
        computeLayout.setLayoutCount = 1;

        VkPushConstantRange pushConstant{};
        pushConstant.offset     = 0;
        pushConstant.size       = sizeof(ComputePushConstants);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pPushConstantRanges    = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(device, &computeLayout, nullptr, &m_PipelineLayout));

        std::string    shaderPath = std::string(PROJECT_ROOT_DIR) + "/MamontEngine/src/Shaders/gradient_color.comp.spv";
        VkShaderModule gradientShader;
        if (!VkPipelines::LoadShaderModule(shaderPath.c_str(), device, &gradientShader))
        {
            fmt::print("Error when building the Gradient shader \n");
        }

        VkShaderModule skyShader;
        std::string    skyShaderPath = std::string(PROJECT_ROOT_DIR) + "/MamontEngine/src/Shaders/sky.comp.spv";
        if (!VkPipelines::LoadShaderModule(skyShaderPath.c_str(), device, &skyShader))
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
        computePipelineCreateInfo.layout = m_PipelineLayout;
        computePipelineCreateInfo.stage  = stageinfo;

        ComputeEffect gradient;
        gradient.Layout = m_PipelineLayout;
        gradient.Name   = "gradient";
        gradient.Data   = {};

        gradient.Data.Data1 = glm::vec4(1, 0, 0, 1);
        gradient.Data.Data2 = glm::vec4(0, 0, 1, 1);

        VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.Pipeline));

        computePipelineCreateInfo.stage.module = skyShader;

        ComputeEffect sky{};
        sky.Layout     = m_PipelineLayout;
        sky.Name       = "Sky";
        sky.Data       = {};
        sky.Data.Data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

        VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.Pipeline));

        m_BackgroundEffects.push_back(gradient);
        m_BackgroundEffects.push_back(sky);

        vkDestroyShaderModule(device, gradientShader, nullptr);
        vkDestroyShaderModule(device, skyShader, nullptr);
        inDeletQueue.PushFunction([=](){
                    vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
                    vkDestroyPipeline(device, sky.Pipeline, nullptr);
                    vkDestroyPipeline(device, gradient.Pipeline, nullptr);
                });
    }
} // namespace MamontEngine
