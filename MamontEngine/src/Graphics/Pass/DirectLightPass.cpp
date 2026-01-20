#include "Graphics/Pass/DirectLightPass.h"
#include "Core/Camera.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Utils/VkPipelines.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include <Utils/Profile.h>
#include "Math/AABB.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"

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
    DirectLightPass::DirectLightPass(VkFormat inCascadeDepthFormat, VkImage inCascadeImage) 
        : m_CascadeImage(inCascadeImage)
    {
        const VkDevice& device = LogicalDevice::GetDevice();

        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            auto layerViewInfo =
                    vkinit::imageviewCreateInfo(inCascadeDepthFormat, inCascadeImage, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

            layerViewInfo.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
            layerViewInfo.subresourceRange.layerCount     = 1;

            VK_CHECK(vkCreateImageView(device, &layerViewInfo, nullptr, &Cascades[i].View));
            std::cerr << "Cascades[" << i << "].View: " << Cascades[i].View << std::endl;
        }
    }
    
    DirectLightPass::~DirectLightPass()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        for (auto& cascade : Cascades)
        {
            vkDestroyImageView(device, cascade.View, nullptr);
        }

        m_CascadePipeline.reset();
    }

    void DirectLightPass::Render(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, const DrawContext &inDrawContext, const glm::mat4 &viewproj)
    {
        PROFILE_ZONE("DirectLightPass::Render");

        constexpr VkExtent2D cascadeExtent = {.width = SHADOWMAP_DIMENSION, .height = SHADOWMAP_DIMENSION};

        uint32_t cascadeIndex{0};
        for (const auto &cascade : Cascades)
        {
            VkUtil::transition_image(cmd, m_CascadeImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

            VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(cascade.View);
            depthAttachment.clearValue.depthStencil   = {1.f, 0};
            depthAttachment.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;

            const VkRenderingInfo renderCascadeInfo = vkinit::rendering_info(cascadeExtent, nullptr, &depthAttachment);

            vkCmdBeginRendering(cmd, &renderCascadeInfo);
            vkCmdSetDepthBias(cmd, 1.25f, 0.f, 1.75f);
            const VkViewport viewport = {.x        = 0.0f,
                                         .y        = 0.0f,
                                         .width    = static_cast<float>(cascadeExtent.width),
                                         .height   = static_cast<float>(cascadeExtent.height),
                                         .minDepth = 0.0f,
                                         .maxDepth = 1.0f};

            const VkRect2D scissor = {.offset = {0, 0}, .extent = cascadeExtent};

            vkCmdSetViewport(cmd, 0, 1, &viewport);
            vkCmdSetScissor(cmd, 0, 1, &scissor);

            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CascadePipeline->Pipeline);

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CascadePipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);

            const auto draw = [&](const RenderObject &r)
            {
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CascadePipeline->Layout, 1, 1, &r.Material->MaterialSet, 0, nullptr);

                constexpr VkDeviceSize offsets[1] = {0};
                vkCmdBindVertexBuffers(cmd, 0, 1, &r.MeshBuffer.VertexBuffer.Buffer, offsets);

                vkCmdBindIndexBuffer(cmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

                const GPUDrawPushConstants push_constants{
                        .WorldMatrix = r.Transform, .VertexBuffer = r.MeshBuffer.VertexBufferAddress, .CascadeIndex = cascadeIndex};

                constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
                vkCmdPushConstants(
                        cmd, m_CascadePipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &push_constants);

                vkCmdDrawIndexed(cmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
            };

            for (const auto &object : inDrawContext.OpaqueSurfaces)
            {
                if (IsVisible(object.Bound, object.Transform, viewproj))
                {
                    draw(object);
                }
            }

            vkCmdEndRendering(cmd);
            VkUtil::transition_image(cmd, m_CascadeImage, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
            ++cascadeIndex;
        }
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

        for (size_t i{0}; i < CASCADECOUNT; i++)
        {
            const float p       = (i + 1) / static_cast<float>(CASCADECOUNT);
            const float log     = minZ * std::pow(ratio, p);
            const float uniform = minZ + range * p;
            const float d       = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i]    = (d - nearClip) / clipRange;
        }

        float lastSplitDist = 0.f;
        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            float                    splitDist      = cascadeSplits[i];
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

            const glm::mat4 inverseCamera = glm::inverse(inCamera->GetProjection() * inCamera->GetViewMatrix());
            for (uint32_t j = 0; j < 8; j++)
            {
                const glm::vec4 invCorner = inverseCamera * glm::vec4(frustumCorners[j], 1.f);
                frustumCorners[j]         = invCorner / invCorner.w;
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
            radius = std::ceil(radius * 16.f) / 16.f;

            const glm::vec3 maxExtents = glm::vec3(radius);
            const glm::vec3 minExtents = -maxExtents;

            const glm::mat4 lightViewMatrix  = glm::lookAt(frustumCenter - inLightDirection * -minExtents.z, frustumCenter, glm::vec3(0.f, 1.f, 0.f));
            const glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.f, maxExtents.z - minExtents.z);

            Cascades[i].SplitDepth        = (inCamera->GetNearClip() + splitDist * clipRange) * -1.f;
            Cascades[i].ViewProjectMatrix = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }
       
    void DirectLightPass::CreatePipeline(std::span<const VkDescriptorSetLayout> inDescriptorLaouts, VkFormat inImageFormat)
    {
        constexpr VkPushConstantRange matrixRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};
        const VkDevice device = LogicalDevice::GetDevice();

        //const std::array<VkDescriptorSetLayout, 2> layouts = {m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

        const VkPipelineLayoutCreateInfo layoutInfo =
                vkinit::pipeline_layout_create_info(static_cast<uint32_t>(inDescriptorLaouts.size()), inDescriptorLaouts.data(), &matrixRange);

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout));

        std::cerr << "Shadow Pipline laoyot: " << layout << std::endl;

        const std::string cascadeShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/cascade_shadow.vert.spv";

        VkShaderModule cascadeShadowShader;
        if (!VkPipelines::LoadShaderModule(cascadeShadowPath.c_str(), device, &cascadeShadowShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }
        const std::string cascadeFragmentShadowPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/cascade_shadow.frag.spv ";

        VkShaderModule cascadeFragmentShadowShader;
        if (!VkPipelines::LoadShaderModule(cascadeFragmentShadowPath.c_str(), device, &cascadeFragmentShadowShader))
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
        pipelineBuilder.SetShaders(cascadeShadowShader, cascadeFragmentShadowShader);
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
        if (shadowPipeline == VK_NULL_HANDLE)
        {
            fmt::println("shadowPipeline == VK_NULL_HANDLE");
        }

        std::cerr << "shadowPipeline: " << shadowPipeline << std::endl;
        std::cerr << "shadowPipeline, Layout: " << layout << std::endl;

        m_CascadePipeline = std::make_unique<PipelineData>(shadowPipeline, layout);

        vkDestroyShaderModule(device, cascadeShadowShader, nullptr);
        vkDestroyShaderModule(device, cascadeFragmentShadowShader, nullptr);

        pipelineBuilder.Clear();
    }

} // namespace MamontEngine