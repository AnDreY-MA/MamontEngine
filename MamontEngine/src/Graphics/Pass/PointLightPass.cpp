#include "Graphics/Pass/PointLightPass.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Utils/VkPipelines.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include <Utils/Profile.h>
#include "Utils/Utilities.h"
#include "Math/AABB.h"
#include "Graphics/Vulkan/Allocator.h"
#include <Utils/VkDestriptor.h>
#include "Core/Camera.h"
#include "Core/Log.h"
#include "Core/Engine.h"

namespace
{
    bool IsVisible(const MamontEngine::AABB &bound, const glm::mat4 inTrasnform, const glm::mat4 &viewproj)
    {
        constexpr std::array<glm::vec3, 8> corners{
                glm::vec3{1, 1, 1},
                glm::vec3{1, 1, -1},
                glm::vec3{1, -1, 1},
                glm::vec3{1, -1, -1},
                glm::vec3{-1, 1, 1},
                glm::vec3{-1, 1, -1},
                glm::vec3{-1, -1, 1},
                glm::vec3{-1, -1, -1},
        };

        const glm::mat4 matrix = viewproj * inTrasnform;

        glm::vec3 min = {1.5, 1.5, 1.5};
        glm::vec3 max = {-1.5, -1.5, -1.5};

        for (int c = 0; c < 8; c++)
        {
            glm::vec4 v = matrix * glm::vec4(bound.Center() + (corners[c] * bound.Extent()), 1.f);

            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
            max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
        }

        if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
        {
            return false;
        }

        return true;
    }
} // namespace


namespace MamontEngine
{
    struct PointShadowConstants
    {
        glm::mat4       WorldMatrix{glm::mat4(0.f)};
        VkDeviceAddress VertexBuffer{0};

        VkDeviceAddress ShadowMapBufferAddress{0};
        uint32_t        BufferIndex{0};
        uint32_t        LightIndex{0};

        float _pad1;
    };

    PointLightPass::PointLightPass(std::array<Texture, MAX_POINT_LIGHT> &inShadowCubeImages) 
        : shadowCubeImages(inShadowCubeImages)
    {
        const VkDevice &device = LogicalDevice::GetDevice();

        m_Buffer.Create(sizeof(ShadowCube) * SHADOW_FACE_NUM * MAX_POINT_LIGHT,
                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                        VMA_MEMORY_USAGE_GPU_ONLY);
        const VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = m_Buffer.Buffer};
        m_Buffer.Address                                  = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);

        for (auto& sBuffer : m_StagingBuffers)
        {
            sBuffer = CreateStagingBuffer(m_Buffer.Info.size);
        }

    }

    PointLightPass::~PointLightPass()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        m_Buffer.Destroy();

        m_Pipeline.reset();

    }

    void PointLightPass::Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj)
    {
        PROFILE_ZONE("PointLightPass::Render");

        constexpr VkExtent2D extent = {.width = 1024, .height = 1024};

        const auto draw = [&](const RenderObject &r, uint32_t lightCount, uint32_t faceNum)
        {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 1, 1, &r.MaterialDescriptorSet, 0, nullptr);

            constexpr VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, &r.MeshBuffer.VertexBuffer.Buffer, offsets);

            vkCmdBindIndexBuffer(cmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

            const PointShadowConstants pushConstants{
                    .WorldMatrix = r.Transform, 
                    .VertexBuffer = r.MeshBuffer.VertexBuffer.Address, 
                    .ShadowMapBufferAddress = m_Buffer.Address,
                    .BufferIndex = faceNum + lightCount * SHADOW_FACE_NUM,
                    .LightIndex = lightCount
            };

            constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(PointShadowConstants))};
            vkCmdPushConstants(cmd, m_Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &pushConstants);

            vkCmdDrawIndexed(cmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
        };

        const auto &contextDevice = MEngine::Get().GetContextDevice();

        for (uint32_t l = 0; l < m_LightCount; ++l)
        {
            const Texture &shadowImage = shadowCubeImages[l];
            VkUtil::transition_image_aspect(
                    cmd, shadowImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
            for (uint32_t i = 0; i < SHADOW_FACE_NUM; ++i)
            {
                VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(contextDevice.PointShadowMapImageViews[i + l * SHADOW_FACE_NUM]);
                depthAttachment.clearValue.depthStencil   = {1.f, 0};
                depthAttachment.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;

                const VkRenderingInfo renderCascadeInfo = vkinit::rendering_info(extent, nullptr, &depthAttachment);

                vkCmdBeginRendering(cmd, &renderCascadeInfo);
                vkCmdSetDepthBias(cmd, 1.25f, 0.f, 1.75f);
                const VkViewport viewport = {.x        = 0.0f,
                                             .y        = 0.0f,
                                             .width    = static_cast<float>(extent.width),
                                             .height   = static_cast<float>(extent.height),
                                             .minDepth = 0.0f,
                                             .maxDepth = 1.0f};

                const VkRect2D scissor = {.offset = {0, 0}, .extent = extent};

                vkCmdSetViewport(cmd, 0, 1, &viewport);
                vkCmdSetScissor(cmd, 0, 1, &scissor);

                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);

                for (const auto &object : inDrawContext.OpaqueSurfaces)
                {
                      if (IsVisible(object.Bound, object.Transform, viewproj))
                      {
                          draw(object, l, i);
                      }
                }

                vkCmdEndRendering(cmd);
            }
            VkUtil::transition_image_aspect(
                    cmd, shadowImage.Image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        }
    }

    void PointLightPass::CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat)
    {
        constexpr VkPushConstantRange matrixRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(PointShadowConstants)};

        const VkDevice device = LogicalDevice::GetDevice();

        const VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info(
            static_cast<uint32_t>(inDescriptorLaouts.size()), inDescriptorLaouts.data(), &matrixRange, 1);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout));

        const std::string pointShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/point_light_shadow_vert.spv";

        VkShaderModule pointShadowShader;
        if (!VkPipelines::LoadShaderModule(pointShadowPath.c_str(), device, &pointShadowShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }
        const std::string pointFragmentShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/point_light_shadow_frag.spv ";

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
        pipelineBuilder.SetDepthFormat(inImageFormat);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
        // pipelineBuilder.SetDepthBiasEnable(VK_TRUE);
        pipelineBuilder.EnableDepthClamp(VK_TRUE);
        pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);

        VkPipeline pointPipeline = pipelineBuilder.BuildPipline(device, 0);
        if (pointPipeline == VK_NULL_HANDLE)
        {
            fmt::println("pointPipeline == VK_NULL_HANDLE");
        }

        std::cerr << "pointPipeline: " << pointPipeline << std::endl;
        std::cerr << "pointPipeline, Layout: " << layout << std::endl;

        m_Pipeline = std::make_unique<PipelineData>(pointPipeline, layout);

        vkDestroyShaderModule(device, pointShadowShader, nullptr);
        vkDestroyShaderModule(device, pointFragmentShadowShader, nullptr);

        pipelineBuilder.Clear();
    }

    void PointLightPass::UpdateLights(const Camera *inCamera, const LightData &inLightData)
    {
        m_LightCount = inLightData.PointLightingCount;

        for (uint32_t i = 0; i < m_LightCount; ++i)
        {
            const auto &light = inLightData.PointLights[i];

            glm::mat4 proj = glm::perspective(glm::radians(90.f), 1.f, inCamera->GetNearClip(), light.Radius);
            proj[1][1] *= -1.0f;
            for (uint32_t f = 0; f < SHADOW_FACE_NUM; ++f)
            {
                glm::mat4 view = glm::mat4(1.0f);
                switch (f)
                {
                    case 0: // POSITIVE_X
                        view = glm::rotate(view, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                        view = glm::rotate(view, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        break;
                    case 1: // NEGATIVE_X
                        view = glm::rotate(view, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                        view = glm::rotate(view, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        break;
                    case 2: // POSITIVE_Y
                        view = glm::rotate(view, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        break;
                    case 3: // NEGATIVE_Y
                        view = glm::rotate(view, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        break;
                    case 4: // POSITIVE_Z
                        view = glm::rotate(view, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                        break;
                    case 5: // NEGATIVE_Z
                        view = glm::rotate(view, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                        break;
                }

                m_ShadowFaceCubes[f + i * SHADOW_FACE_NUM] = proj * view * glm::translate(glm::mat4(1.f), -light.Position);
            }
        }

        const auto currentFrame = MEngine::Get().GetContextDevice().GetFrame();
        CopyDataToDynamicBuffer(&m_Buffer, (void *)m_ShadowFaceCubes.data(), &m_StagingBuffers[currentFrame]);
    }
}