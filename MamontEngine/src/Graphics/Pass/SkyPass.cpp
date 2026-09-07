#include "SkyPass.h"
#include "Utils//VkPipelines.h"
#include "Utils//VkDestriptor.h"
#include "Utils//VkInitializers.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Resources/Models/Mesh.h"
#include <Utils/Profile.h>
#include "Core/Engine.h"

namespace MamontEngine
{
    SkyPass::SkyPass()
    {
        const std::string cubePath = DEFAULT_ASSETS_DIRECTORY + "cube.glb";

        m_Skybox = std::make_unique<MeshModel>(0, cubePath);
        m_Skybox->Draw(m_SkyboxContext);

        const RenderObject &object = m_SkyboxContext.OpaqueSurfaces[0];
        const VkDeviceAddress     vertexAddress = object.MeshBuffer.VertexBuffer.Address;

        MEngine::Get().GetContextDevice().CreatePrefilteredCubeTexture(vertexAddress,
                                                     [object](VkCommandBuffer cmd)
                                                     {
                                                         constexpr VkDeviceSize offsets[1] = {0};
                                                         vkCmdBindVertexBuffers(cmd, 0, 1, &object.MeshBuffer.VertexBuffer.Buffer, offsets);

                                                         vkCmdBindIndexBuffer(cmd, object.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

                                                         vkCmdDrawIndexed(cmd, object.IndexCount, 1, object.FirstIndex, 0, 0);
                                                     });
    }

    SkyPass::~SkyPass()
    {
    }

    void SkyPass::CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, std::pair<VkFormat, VkFormat> inImageFormats)
    {
        const VkDevice& device = LogicalDevice::GetDevice();

        m_ImageFormats = inImageFormats;

        constexpr VkPushConstantRange matrixRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};


        const VkPipelineLayoutCreateInfo skyboxlayoutInfo =
                vkinit::pipeline_layout_create_info(inDescriptorLaouts.size(), inDescriptorLaouts.data(), &matrixRange, 1);

        VkPipelineLayout skyboxLayout;
        VK_CHECK(vkCreatePipelineLayout(device, &skyboxlayoutInfo, nullptr, &skyboxLayout));

        const std::string skyboxPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/skybox.frag.spv";

        VkShaderModule skyboxFragShader;
        if (!VkPipelines::LoadShaderModule(skyboxPath.c_str(), device, &skyboxFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string skyboxVertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/skybox.vert.spv";
        VkShaderModule    skyboxVertexShader;
        if (!VkPipelines::LoadShaderModule(skyboxVertexShaderPath.c_str(), device, &skyboxVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }

        std::cerr << "skyboxFragShader: " << skyboxFragShader << "\n";
        std::cerr << "skyboxVertexShader: " << skyboxVertexShader << "\n";

        VkPipelineCacheCreateInfo pipelineCacheInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        VkPipelineCache skyPipelineCache{VK_NULL_HANDLE};
        VK_CHECK(vkCreatePipelineCache(device, &pipelineCacheInfo, nullptr, &skyPipelineCache));

        const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.Clear();
        pipelineBuilder.SetShaders(skyboxVertexShader, skyboxFragShader);
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetDepthFormat(inImageFormats.second);
        pipelineBuilder.SetColorAttachmentFormat(inImageFormats.first);
        pipelineBuilder.SetVertexInput(vertexInputInfo);
        pipelineBuilder.SetLayout(skyboxLayout);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.EnableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.DisableBlending();
        pipelineBuilder.SetCache(skyPipelineCache);
        pipelineBuilder.SetMultisamplingNone();


        m_Pipeline = std::make_unique<PipelineData>(pipelineBuilder.BuildPipline(device), skyboxLayout, skyPipelineCache);

        vkDestroyShaderModule(device, skyboxFragShader, nullptr);
        vkDestroyShaderModule(device, skyboxVertexShader, nullptr);
    }

    void SkyPass::Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
    {
        PROFILE_ZONE("SkyPass");

        const RenderObject &object = m_SkyboxContext.OpaqueSurfaces[0];

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Pipeline);

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout,
                                0,
                                1,
                                &globalDescriptor,
                                0,
                                nullptr);

        constexpr VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, &object.MeshBuffer.VertexBuffer.Buffer, offsets);

        vkCmdBindIndexBuffer(cmd, object.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

        const GPUDrawPushConstants push_constants{
                .WorldMatrix  = glm::mat4(1.f),
                .VertexBuffer = object.MeshBuffer.VertexBuffer.Address,
        };

        constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
        vkCmdPushConstants(cmd, m_Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &push_constants);

        vkCmdDrawIndexed(cmd, object.IndexCount, 1, object.FirstIndex, 0, 0);

    }
} // namespace MamontEngine
