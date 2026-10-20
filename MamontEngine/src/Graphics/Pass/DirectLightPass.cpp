#include "Graphics/Pass/DirectLightPass.h"
#include "Core/Camera.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Utils/VkPipelines.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include <Utils/Profile.h>
#include "Math/AABB.h"
#include "Core/Engine.h"
#include "Graphics/Frustum.h"
#include "Graphics/Shader/ShaderModule.h"

namespace MamontEngine
{
    DirectLightPass::DirectLightPass(VkFormat inCascadeDepthFormat, VkImage inCascadeImage) 
        : m_CascadeImage(inCascadeImage), m_CascadeFormat(inCascadeDepthFormat)
    {
        const VkDevice& device = LogicalDevice::GetDevice();
        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            auto layerViewInfo =
                    vkinit::imageviewCreateInfo(inCascadeDepthFormat, inCascadeImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

            layerViewInfo.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
            layerViewInfo.subresourceRange.layerCount     = 1;

            VK_CHECK(vkCreateImageView(device, &layerViewInfo, nullptr, &m_CascadeImageViews[i]));
            std::cerr << "Cascades[" << i << "].View: " << m_CascadeImageViews[i] << std::endl;
        }
    }
    
    DirectLightPass::~DirectLightPass()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        for (auto &cascadeView : m_CascadeImageViews)
        {
            vkDestroyImageView(device, cascadeView, nullptr);
        }

        m_Pipeline.reset();
    }

    void DirectLightPass::Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj)
    {
        PROFILE_ZONE("DirectLightPass::Render");

        const VkCommandBufferInheritanceRenderingInfo inheritanceDynamicInfo = {
            .sType                   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO,
            .viewMask                = 0,
            .colorAttachmentCount    = 0,
            .depthAttachmentFormat   = m_CascadeFormat,
            .rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT
        };

        const VkCommandBufferInheritanceInfo inheritanceInfo = {
                .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
                .pNext       = &inheritanceDynamicInfo,
                .renderPass  = VK_NULL_HANDLE,
                .subpass     = 0,
                .framebuffer = VK_NULL_HANDLE,
        };

        const VkCommandBufferBeginInfo cmdSecondaryBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, &inheritanceInfo);

        constexpr VkExtent2D cascadeExtent = {.width = SHADOWMAP_DIMENSION  , .height = SHADOWMAP_DIMENSION};

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdSecondaryBeginInfo));

        const auto draw = [&](const RenderObject &r, uint32_t cascadeIndex)
        {
            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 1, 1, &r.MaterialDescriptorSet, 0, nullptr);

            vkCmdBindIndexBuffer(cmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

            const GPUDrawPushConstants push_constants{
                    .WorldMatrix = r.Transform, .VertexBuffer = r.MeshBuffer.VertexBuffer.Address, .CascadeIndex = cascadeIndex};

            constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
            vkCmdPushConstants(cmd, m_Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &push_constants);

            vkCmdDrawIndexed(cmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
        };


        VkUtil::transition_image(cmd, m_CascadeImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Pipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);

        constexpr VkViewport viewport = {.x        = 0.0f,
                                         .y        = 0.0f,
                                         .width    = static_cast<float>(cascadeExtent.width),
                                         .height   = static_cast<float>(cascadeExtent.height),
                                         .minDepth = 0.0f,
                                         .maxDepth = 1.0f};

        constexpr VkRect2D scissor = {.offset = {0, 0}, .extent = cascadeExtent};

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
        vkCmdSetDepthBias(cmd, 1.25f, 0.f, 1.75f);

        for (uint32_t i = 0; i < m_CascadeImageViews.size(); ++i)
        {
            const auto frustumPlanes = ExtractFrustumPlanes(m_Cascades.ViewProjectMatrixs[i]);

            const VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(m_CascadeImageViews[i], VK_ATTACHMENT_LOAD_OP_LOAD);

            const VkRenderingInfo renderCascadeInfo = vkinit::rendering_info(cascadeExtent, nullptr, &depthAttachment);

            vkCmdBeginRendering(cmd, &renderCascadeInfo);
            
            for (const auto &object : inDrawContext.OpaqueSurfaces)
            {
                if (IsNodeVisible(frustumPlanes, object.Transform, object.SphereBound))
                {
                    draw(object, i);
                }
            }

            vkCmdEndRendering(cmd);
        }
        VkUtil::transition_image(cmd, m_CascadeImage, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

        VK_CHECK(vkEndCommandBuffer(cmd));
    }

    void DirectLightPass::UpdateCascade(const Camera *inCamera, const glm::vec3 &inLightDirection)
    {
        std::array<float, CASCADECOUNT> cascadeSplits{};

        const float nearClip{inCamera->GetNearClip()};
        const float farClip{inCamera->GetFarClip()};
        const float clipRange{farClip - nearClip};

        const float minZ = nearClip;
        const float maxZ = nearClip + clipRange;

        const float range = maxZ - minZ;
        const float ratio = maxZ / minZ;

        constexpr float cascadeSplitLambda{0.95f};
        const glm::mat4 inverseCamera = glm::inverse(inCamera->GetProjection() * inCamera->GetViewMatrix());
        const glm::vec3 lightDirection = glm::normalize(inLightDirection);

        float lastSplitDist = 0.f;
        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            {
                const float p       = (i + 1) / static_cast<float>(CASCADECOUNT);
                const float log     = minZ * std::pow(ratio, p);
                const float uniform = minZ + range * p;
                const float d       = cascadeSplitLambda * (log - uniform) + uniform;
                cascadeSplits[i]    = (d - nearClip) / clipRange;
            }

            const float                    splitDist      = cascadeSplits[i];
            std::array<glm::vec3, 8> frustumCorners = {
                    glm::vec3(-1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, -1.0f, 1.0f),
                    glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            for (uint32_t j = 0; j < 8; j++)
            {
                glm::vec4 corner = inverseCamera * glm::vec4(frustumCorners[j], 1.f);
                corner /= corner.w;
                frustumCorners[j]         = glm::vec3(corner);
            }

            for (uint32_t j = 0; j < 4; j++)
            {
                const glm::vec3 dist  = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j]     = frustumCorners[j] + (dist * lastSplitDist);
            }

            glm::vec3 frustumCenter = glm::vec3(0.f);
            for (uint32_t j = 0; j < 8; j++)
            {
                frustumCenter += frustumCorners[j];
            }
            frustumCenter /= 8.f;

            float radius = 0.f;
            for (uint32_t j = 0; j < 8; j++)
            {
                const float distance = glm::length(frustumCorners[j] - frustumCenter);
                radius               = glm::max(radius, distance);
            }
            
            const glm::mat4 lightViewMatrix = glm::lookAtRH(frustumCenter - lightDirection * radius, frustumCenter, glm::vec3(0.f, 1.f, 0.f));
            glm::mat4 lightOrthoMatrix = glm::orthoRH_ZO(-radius, radius, -radius, radius, 0.005f, radius * 2.f);
            lightOrthoMatrix[1][1] *= -1.0f;
            
            m_Cascades.DepthSpilts[i] = (nearClip + splitDist * clipRange) * -1.f;
            m_Cascades.ViewProjectMatrixs[i]             = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = splitDist;
        }
    }
       
    void DirectLightPass::CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat)
    {
        constexpr VkPushConstantRange matrixRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};
        const VkDevice device = LogicalDevice::GetDevice();

        const VkPipelineLayoutCreateInfo layoutInfo =
                vkinit::pipeline_layout_create_info(static_cast<uint32_t>(inDescriptorLaouts.size()), inDescriptorLaouts.data(), &matrixRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout));

        const std::string vertPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/cascade_shadow.vert";
        const std::string fragPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/cascade_shadow.frag";

        ShaderModule vertModule(device, vertPath, EShaderStage::Vertex);
        ShaderModule fragModule(device, fragPath, EShaderStage::Fragment);

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
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.SetDepthFormat(inImageFormat);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
        // pipelineBuilder.SetDepthBiasEnable(VK_TRUE);
        pipelineBuilder.EnableDepthClamp(VK_TRUE);
        pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);

        VkPipeline shadowPipeline = pipelineBuilder.BuildPipline(device, 0);

        std::cerr << "Direct shadowPipeline: " << shadowPipeline << std::endl;
        std::cerr << "Direct shadowPipeline, Layout: " << layout << std::endl;

        m_Pipeline = std::make_unique<PipelineData>(shadowPipeline, layout);
    }

} // namespace MamontEngine