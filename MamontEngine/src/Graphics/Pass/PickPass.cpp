#include "PickPass.h"
#include "Utils/VkPipelines.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include <Utils/Profile.h>
#include "Utils/Utilities.h"
#include "Core/Log.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include <Utils/VkDestriptor.h>
#include "Core/Engine.h"

namespace MamontEngine
{
    struct alignas(16) OutlinePushConstants
    {
        uint32_t ObjectId = 1;
        float    OutlineWidth     = 0.5f;
        float    _pad0             = 0.0f;
        float    _pad1             = 0.0f;
        glm::vec4        OutlineColor = {1.0f, 0.95f, 0.15f, 1.0f};
    };

    PickPass::PickPass()
    {
        const auto             &device = LogicalDevice::GetDevice();
        DescriptorLayoutBuilder layoutBuilder{};
        layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        m_IdDescriptorSetLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_FRAGMENT_BIT);

        auto &contextDevice = MEngine::Get().GetContextDevice();

        m_IdDescriptorSet = contextDevice.GlobalDescriptorAllocator.Allocate(device, m_IdDescriptorSetLayout);

        DescriptorWriter writer;
        writer.Clear();
        writer.WriteImage(0,
                          contextDevice.IdTexture.ImageView,
                          contextDevice.IdTexture.Sampler,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        writer.UpdateSet(device, m_IdDescriptorSet);
    }

    PickPass::~PickPass()
    {

    }
    void PickPass::CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat)
    {

        // Outline Pipeline
/*        {
            constexpr VkPushConstantRange matrixRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(OutlinePushConstants)};

            const VkDevice device = LogicalDevice::GetDevice();

            const VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info(1, &m_IdDescriptorSetLayout, &matrixRange, 1);

            VkPipelineLayout layout;
            VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout));

            const std::string pointShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/outline.vert.spv";

            VkShaderModule pointShadowShader;
            if (!VkPipelines::LoadShaderModule(pointShadowPath.c_str(), device, &pointShadowShader))
            {
                fmt::println("Error when building the triangle fragment shader module");
            }
            const std::string pointFragmentShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/outline.frag.spv ";

            VkShaderModule pointFragmentShadowShader;
            if (!VkPipelines::LoadShaderModule(pointFragmentShadowPath.c_str(), device, &pointFragmentShadowShader))
            {
                fmt::println("Error when building the triangle fragment shader module");
            }
            const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
            const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
            const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                    vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

            VkPipelines::PipelineBuilder pipelineBuilder;
            pipelineBuilder.Clear();
            pipelineBuilder.SetVertexInput(vertexInputInfo);
            pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
            pipelineBuilder.SetLayout(layout);
            pipelineBuilder.SetShaders(pointShadowShader, pointFragmentShadowShader);
            pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
            pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
            pipelineBuilder.SetDepthFormat(MEngine::Get().GetContextDevice().IdTexture.ImageFormat);
            pipelineBuilder.SetMultisamplingNone();
            pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            pipelineBuilder.DisableBlending();
            // pipelineBuilder.SetDepthBiasEnable(VK_TRUE);
            pipelineBuilder.EnableDepthClamp(VK_TRUE);
            pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);

            VkPipeline outlinePipeline = pipelineBuilder.BuildPipline(device, 0);
            if (outlinePipeline == VK_NULL_HANDLE)
            {
                fmt::println("outlinePipeline == VK_NULL_HANDLE");
            }

            std::cerr << "outlinePipeline: " << outlinePipeline << std::endl;
            std::cerr << "outlinePipeline, Layout: " << layout << std::endl;

            m_OutlinePipeline = std::make_unique<PipelineData>(outlinePipeline, layout);

            vkDestroyShaderModule(device, pointShadowShader, nullptr);
            vkDestroyShaderModule(device, pointFragmentShadowShader, nullptr);

            pipelineBuilder.Clear();
        }*/
    }
} // namespace MamontEngine
