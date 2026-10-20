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
#include <Graphics/Shader/ShaderModule.h>

namespace
{
    using namespace MamontEngine;

    void UploadDataToBuffer(VkCommandBuffer cmd, void* newData, AllocatedBuffer& targetBuffer, AllocatedBuffer& stagingBuffer, size_t size, size_t offset)
    {
        const auto bufferBarrier1 = VkBufferMemoryBarrier2{
                .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT,
                .dstStageMask  = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .buffer        = targetBuffer.Buffer,
                .offset        = 0,
                .size          = VK_WHOLE_SIZE,
        };

        const auto dependencyInfo = VkDependencyInfo{
                .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers    = &bufferBarrier1,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo);

        auto *mappedData = reinterpret_cast<uint8_t *>(stagingBuffer.Info.pMappedData);
        memcpy((void *)&mappedData[offset], newData, size);

        const auto region = VkBufferCopy2{
                .sType     = VK_STRUCTURE_TYPE_BUFFER_COPY_2,
                .srcOffset = (VkDeviceSize)offset,
                .dstOffset = (VkDeviceSize)offset,
                .size      = size,
        };
        const auto bufCopyInfo = VkCopyBufferInfo2{
                .sType       = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2,
                .srcBuffer   = stagingBuffer.Buffer,
                .dstBuffer   = targetBuffer.Buffer,
                .regionCount = 1,
                .pRegions    = &region,
        };

        vkCmdCopyBuffer2(cmd, &bufCopyInfo);

        const auto bufferBarrier2 = VkBufferMemoryBarrier2{
                .sType         = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT,
                .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
                .dstStageMask  = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                .dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT,
                .buffer        = targetBuffer.Buffer,
                .offset        = 0,
                .size          = VK_WHOLE_SIZE,
        };
        const auto dependencyInfo2 = VkDependencyInfo{
                .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers    = &bufferBarrier2,
        };
        vkCmdPipelineBarrier2(cmd, &dependencyInfo2);
    }
}

namespace MamontEngine
{
    struct PointShadowConstants
    {
        glm::mat4       WorldMatrix{glm::mat4(1.f)};
        VkDeviceAddress VertexBuffer{0};

        VkDeviceAddress ShadowMapBufferAddress{0};
        uint32_t        BufferIndex{0};
        uint32_t        LightIndex{0};
    };

    struct CubeFace
    {
        glm::vec3 Direction{glm::vec3(0.f)};
        glm::vec3 Up{glm::vec3(0.f)};
    };

    PointLightPass::PointLightPass(std::array<Texture, MAX_POINT_LIGHT> &inShadowCubeImages) 
        : shadowCubeImages(inShadowCubeImages)
    {
        const VkDevice &device = LogicalDevice::GetDevice();

        constexpr VkDeviceSize shadowBufferSize = sizeof(glm::mat4) * MAX_POINT_LIGHT * SHADOW_FACE_NUM;

        for (auto& buffer : m_Buffer)
        {
            buffer.Create(shadowBufferSize,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                            VMA_MEMORY_USAGE_GPU_ONLY);
            const VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer.Buffer};
            buffer.Address                                    = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);
        }

        
        for (auto& sBuffer : m_StagingBuffers)
        {
            sBuffer = CreateStagingBuffer(shadowBufferSize);
        }

    }

    PointLightPass::~PointLightPass()
    {
        const VkDevice device = LogicalDevice::GetDevice();
        for (auto& sBuffer : m_StagingBuffers)
        {
            sBuffer.Destroy();
        }

        for (auto& buffer : m_Buffer)
        {
            buffer.Destroy();
        }


        m_Pipeline.reset();

    }

    void PointLightPass::Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj)
    {
        PROFILE_ZONE("PointLightPass::Render");

        VkCommandBufferInheritanceRenderingInfo inheritanceDynamicInfo{};
        inheritanceDynamicInfo.sType                 = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
        inheritanceDynamicInfo.colorAttachmentCount  = 0;
        inheritanceDynamicInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;
        inheritanceDynamicInfo.viewMask              = 0;
        inheritanceDynamicInfo.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;

        const VkCommandBufferInheritanceInfo inheritanceInfo = {
                .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
                .pNext       = &inheritanceDynamicInfo,
                .renderPass  = VK_NULL_HANDLE,
                .subpass     = 0,
                .framebuffer = VK_NULL_HANDLE,
        };

        VkCommandBufferBeginInfo cmdSecondaryBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        cmdSecondaryBeginInfo.pInheritanceInfo         = &inheritanceInfo;

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdSecondaryBeginInfo));

        constexpr VkExtent2D extent = {.width = SHADOW_MAP_FACE_IMAGE_SIZE, .height = SHADOW_MAP_FACE_IMAGE_SIZE};
        const auto           currentFrame = MEngine::Get().GetContextDevice().GetFrame();

        UploadDataToBuffer(cmd, (void *)m_ShadowFaceCubes.data(), m_Buffer[currentFrame], m_StagingBuffers[currentFrame], sizeof(m_ShadowFaceCubes), 0);

        const auto draw = [&](const RenderObject &r, uint32_t lightCount, uint32_t faceIndex)
        {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 1, 1, &r.MaterialDescriptorSet, 0, nullptr);

            vkCmdBindIndexBuffer(cmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

            const PointShadowConstants pushConstants{
                    .WorldMatrix = r.Transform, 
                    .VertexBuffer = r.MeshBuffer.VertexBuffer.Address, 
                    .ShadowMapBufferAddress = m_Buffer[currentFrame].Address,
                    .BufferIndex = lightCount * SHADOW_FACE_NUM + faceIndex,
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
            
            for (uint32_t faceIndex = 0; faceIndex < SHADOW_FACE_NUM; ++faceIndex)
            {
                VkUtil::PipelineBarrier(
                        cmd, shadowImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
                VkRenderingAttachmentInfo depthAttachment =
                        vkinit::depth_attachment_info(contextDevice.PointShadowMapImageViews[l * SHADOW_FACE_NUM + faceIndex]);
                depthAttachment.clearValue.depthStencil   = {1.f, 0};
                depthAttachment.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;

                const VkRenderingInfo renderCascadeInfo = vkinit::rendering_info(extent, nullptr, &depthAttachment);

                vkCmdBeginRendering(cmd, &renderCascadeInfo);
                //vkCmdSetDepthBias(cmd, 1.25f, 0.f, 1.75f);
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
                    draw(object, l, faceIndex);
                }

                vkCmdEndRendering(cmd);
                VkUtil::PipelineBarrier(cmd,
                                                shadowImage.Image,
                                                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            }
            
        }

        VK_CHECK(vkEndCommandBuffer(cmd));
    }

    void PointLightPass::CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat)
    {
        constexpr VkPushConstantRange matrixRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(PointShadowConstants)};

        const VkDevice& device = LogicalDevice::GetDevice();

        const VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info(
            static_cast<uint32_t>(inDescriptorLaouts.size()), inDescriptorLaouts.data(), &matrixRange, 1);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout));

        const std::string pointShadowVertPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/point_light_shadow_vert.glsl";
        const std::string pointFragmentShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/point_light_shadow_frag.glsl";

        ShaderModule fragModule(device, pointFragmentShadowPath, EShaderStage::Fragment);
        ShaderModule vertModule(device, pointShadowVertPath, EShaderStage::Vertex);

        const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.Clear();
        pipelineBuilder.SetVertexInput(vertexInputInfo);
        pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.SetLayout(layout);
        pipelineBuilder.SetShaders(vertModule.GetModule(), fragModule.GetModule());
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_LESS);
        pipelineBuilder.SetDepthFormat(VK_FORMAT_D32_SFLOAT);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthClamp(VK_TRUE);
        pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);

        VkPipeline pointPipeline = pipelineBuilder.BuildPipline(device, 0);

        std::cerr << "pointPipeline: " << pointPipeline << std::endl;
        std::cerr << "pointPipeline, Layout: " << layout << std::endl;

        m_Pipeline = std::make_unique<PipelineData>(pointPipeline, layout);

        pipelineBuilder.Clear();
    }

    void PointLightPass::UpdateLights(const Camera *inCamera, const LightData &inLightData)
    {
        m_LightCount = std::min(inLightData.PointLightingCount, MAX_POINT_LIGHT);

        if (m_LightCount == 0)
            return;

        constexpr std::array<CubeFace, SHADOW_FACE_NUM> CubeFaces = {
                CubeFace{{1, 0, 0}, {0, -1, 0}},  // +X
                CubeFace{{-1, 0, 0}, {0, -1, 0}}, // -X
                CubeFace{{0, 1, 0}, {0, 0, 1}},   // +Y
                CubeFace{{0, -1, 0}, {0, 0, -1}}, // -Y
                CubeFace{{0, 0, 1}, {0, -1, 0}},  // +Z
                CubeFace{{0, 0, -1}, {0, -1, 0}}, // -Z
        };

        constexpr float PointShadowNearPlane{0.05f};

        for (uint32_t i = 0; i < m_LightCount; ++i)
        {
            const auto &light = inLightData.PointLights[i];

            glm::mat4 proj = glm::perspectiveRH_ZO(glm::radians(90.f), 1.f, PointShadowNearPlane, light.Radius);
            proj[1][1] *= -1.0f;

            for (uint32_t faceIndex = 0; faceIndex < SHADOW_FACE_NUM; ++faceIndex)
            {
                const auto &face = CubeFaces[faceIndex];

                const glm::mat4 view = glm::lookAtRH(light.Position, light.Position + face.Direction, face.Up);

                m_ShadowFaceCubes[i * SHADOW_FACE_NUM + faceIndex] = proj * view;
            }
/*           for (uint32_t f = 0; f < SHADOW_FACE_NUM; ++f)
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

                m_ShadowFaceCubes[i * SHADOW_FACE_NUM + f] = proj * view * glm::translate(glm::mat4(1.f), -light.Position);
            }*/
        }

        //const auto currentFrame = MEngine::Get().GetContextDevice().GetFrame();
        //CopyDataToDynamicBuffer(&m_Buffer[currentFrame], m_ShadowFaceCubes.data(), &m_StagingBuffers[currentFrame]);
    }
}