#include "Renderer.h"
#include "Core/ContextDevice.h"

#include "Graphics/ImGuiRenderer.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include "Utils/VkPipelines.h"
#include "Core/Window.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/glm.hpp"

#include "Graphics/Vulkan/Allocator.h"
#include "Utils/Profile.h"
#include "vulkan/vk_enum_string_helper.h"
#include "Graphics/Vulkan/ImmediateContext.h"
#include "Graphics/Resources/Models/Model.h"
#include <execution>
#include "Core/Log.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Pass/DirectLightPass.h"
#include "Core/JobSystem.h"
#include "Graphics/DebugRenderer.h"
// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    Renderer::Renderer(VkContextDevice &inDeviceContext, const std::shared_ptr<WindowCore> &inWindow) 
        : m_DeviceContext(inDeviceContext), m_Window(inWindow)
    {
        InitPipelines();

        m_DirectLightPass = std::make_unique<DirectLightPass>(m_DeviceContext.CascadeDepthImage.ImageFormat, m_DeviceContext.CascadeDepthImage.Image);

        const std::array<VkDescriptorSetLayout, 2> layouts = {m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};
        m_DirectLightPass->CreatePipeline(layouts, m_DeviceContext.CascadeDepthImage.ImageFormat);

        DebugRenderer::Init();
    }

    Renderer::~Renderer()
    {
        m_Skybox.reset();
        m_SceneRenderer.reset();
        DestroyPipelines();
        DebugRenderer::Destroy();
    }

    void Renderer::InitSceneRenderer(const std::shared_ptr<Camera> &inMainCamera, const std::shared_ptr<Scene> &inScene)
    {
        m_SceneRenderer = std::make_shared<SceneRenderer>(inMainCamera, inScene);
        const std::string cubePath = DEFAULT_ASSETS_DIRECTORY + "cube.glb";
        m_Skybox                   = std::make_unique<MeshModel>(0, cubePath);

        VkDeviceAddress vertexAddress{0};
        {
            DrawContext skyboxContext;
            m_Skybox->Draw(skyboxContext);
            const RenderObject &object = skyboxContext.OpaqueSurfaces[0];
            vertexAddress = object.MeshBuffer.VertexBufferAddress;
        }

        m_DeviceContext.CreatePrefilteredCubeTexture(vertexAddress, [&](VkCommandBuffer cmd) {
                    DrawContext skyboxContext;
                    m_Skybox->Draw(skyboxContext);
                    const RenderObject &object = skyboxContext.OpaqueSurfaces[0];

                    constexpr VkDeviceSize offsets[1] = {0};
                    vkCmdBindVertexBuffers(cmd, 0, 1, &object.MeshBuffer.VertexBuffer.Buffer, offsets);

                    vkCmdBindIndexBuffer(cmd, object.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

                    vkCmdDrawIndexed(cmd, object.IndexCount, 1, object.FirstIndex, 0, 0);
            });
    }

    void Renderer::InitImGuiRenderer()
    {
        m_ImGuiRenderer = std::make_unique<ImGuiRenderer>(m_DeviceContext, m_Window->GetWindow(), m_DeviceContext.Swapchain.GetImageFormat());
    }

    void Renderer::InitPipelines()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        const std::pair<VkFormat, VkFormat> inImageFormats{m_DeviceContext.DrawImage.ImageFormat, m_DeviceContext.DepthImage.ImageFormat};

        fmt::println("DrawImage.ImageFormat: {}", string_VkFormat(inImageFormats.first));
        fmt::println("DepthImage.ImageFormat: {}", string_VkFormat(inImageFormats.second));

        const std::array<VkDescriptorSetLayout, 2> laouts{m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

        m_RenderPipeline = std::make_shared<RenderPipeline>(device, laouts, inImageFormats);

        m_DeviceContext.RenderPipeline = m_RenderPipeline;

        InitPickPipepline();

    }

    void Renderer::InitPickPipepline()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        const std::string pickingPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/picking.frag.spv";

        VkShaderModule pickingFragShader;
        if (!VkPipelines::LoadShaderModule(pickingPath.c_str(), device, &pickingFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
            return;
        }

        const std::string pickingShaderVertPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/picking.vert.spv";
        VkShaderModule    pickingVertShader;
        if (!VkPipelines::LoadShaderModule(pickingShaderVertPath.c_str(), device, &pickingVertShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
            return;
        }

        std::cerr << "pickingFragShader: " << pickingFragShader << "\n";
        std::cerr << "pickingVertShader: " << pickingVertShader << "\n";

        const std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, pickingVertShader),
                vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, pickingFragShader)};

        VkPipelineLayout pickingLayout;

        // Create Layout
        {
            constexpr VkPushConstantRange pushConstRange = {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset     = 0,
                    .size       = sizeof(GPUDrawPushConstants),
            };

            const VkDescriptorSetLayout layouts[] = {m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

            const VkPipelineLayoutCreateInfo picking_layout_info = vkinit::pipeline_layout_create_info(2, layouts, &pushConstRange);

            VK_CHECK(vkCreatePipelineLayout(device, &picking_layout_info, nullptr, &pickingLayout));
        }

        const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetShaders(pickingVertShader, pickingFragShader);
        pipelineBuilder.SetLayout(pickingLayout);
        pipelineBuilder.SetVertexInput(vertexInputInfo);
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.SetMultisamplingNone();

        pipelineBuilder.DisableBlending();
        pipelineBuilder.m_ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT;

        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
        pipelineBuilder.m_Rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

        pipelineBuilder.SetColorAttachmentFormat(m_DeviceContext.PickingImages[0].ImageFormat);
        pipelineBuilder.SetDepthFormat(m_DeviceContext.DepthImage.ImageFormat);

        VkPipeline pickingPipeline = pipelineBuilder.BuildPipline(device, 1);
        m_PickingPipeline          = std::make_unique<PipelineData>(pickingPipeline, pickingLayout);
        std::cerr << "PickingPipeline: " << pickingPipeline << std::endl;
        std::cerr << "PickingPipeline, Layout: " << pickingLayout << std::endl;

        vkDestroyShaderModule(device, pickingFragShader, nullptr);
        vkDestroyShaderModule(device, pickingVertShader, nullptr);
    }

    void Renderer::Render()
    {
        PROFILE_ZONE("Renderer::Render");

        if (!m_DeviceContext.BeginFrame())
        {
            return;
        }

        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();

        const VkExtent2D& swapchainExtent = m_DeviceContext.Swapchain.GetExtent();
        m_DrawExtent                     = swapchainExtent;
        const uint32_t swapchainImageIndex    = m_DeviceContext.Swapchain.GetCurrentImageIndex();

        VkCommandBuffer cmd = currentFrame.MainCommandBuffer;
        UpdateUniformBuffers();

        VK_CHECK(vkResetCommandBuffer(cmd, 0));

        const VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkUtil::transition_image(cmd, m_DeviceContext.DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        VkImage currentSwapchainImage = m_DeviceContext.Swapchain.GetImageAt(swapchainImageIndex);
        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkUtil::copy_image_to_image(cmd, m_DeviceContext.DrawImage.Image, currentSwapchainImage, m_DrawExtent, m_DeviceContext.Swapchain.GetExtent());

        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkCommandBuffer uiCommandBuffer = currentFrame.UICommandBuffer;
        m_ImGuiRenderer->Draw(uiCommandBuffer, m_DeviceContext.Swapchain.GetImageView(swapchainImageIndex), m_DrawExtent);

        {
            VkUtil::transition_image(
                    cmd, m_DeviceContext.PickingImages[swapchainImageIndex].Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            DrawPickingPass(cmd, swapchainImageIndex);
            VkUtil::transition_image(cmd,
                                     m_DeviceContext.PickingImages[swapchainImageIndex].Image,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


        const std::array<VkCommandBuffer, 1> commandBuffers{uiCommandBuffer};

        vkCmdExecuteCommands(cmd, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

        VK_CHECK(vkEndCommandBuffer(cmd));

        m_DeviceContext.EndFrame(cmd);
    }

    void Renderer::DrawMain(VkCommandBuffer inCmd)
    {
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "Draw Main");

        RenderCascadeShadow(inCmd);

        {
            const auto start = std::chrono::high_resolution_clock::now();

            DrawGeometry(inCmd);

            const auto end     = std::chrono::high_resolution_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            m_Stats.MeshDrawTime = elapsed.count() / 1000.f;
        }
    }

    void Renderer::DrawGeometry(VkCommandBuffer inCmd)
    {
        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();
        PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Draw Geometry");

        const GPUSceneData &sceneData = m_SceneRenderer->GetGPUSceneData();

        const VkExtent2D &extent = m_Window->GetExtent();

        constexpr VkClearValue                    clearColor      = {.color = {0.0f, 0.0f, 0.0f, 0.0f}};
        constexpr VkClearValue           clearDepth      = {.depthStencil = {1.0f, 0}};
        const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(m_DeviceContext.DrawImage.ImageView, &clearColor);
        VkRenderingAttachmentInfo       depthAttachment = vkinit::depth_attachment_info(m_DeviceContext.DepthImage.ImageView);
        depthAttachment.clearValue                   = clearDepth;

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(inCmd, &renderInfo);

        SetViewportScissor(inCmd, m_DrawExtent);
        vkCmdSetDepthBias(inCmd, 0, 0, 0);

        DrawSkybox(inCmd);

        {
            PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Scene Render");
            m_SceneRenderer->Render(inCmd, currentFrame.GlobalDescriptor, m_RenderPipeline.get());
        }
        vkCmdEndRendering(inCmd);

        /*stats.DrawCallCount = 0;
        stats.TriangleCount = 0;*/
    }

    void Renderer::SetViewportScissor(VkCommandBuffer cmd, const VkExtent2D &inExtent) const
    {
        const VkViewport viewport = {.x        = 0.0f,
                                     .y        = 0.0f,
                                     .width    = static_cast<float>(inExtent.width),
                                     .height   = static_cast<float>(inExtent.height),
                                     .minDepth = 0.0f,
                                     .maxDepth = 1.0f};

        const VkRect2D scissor = {.offset = {0, 0}, .extent = inExtent};

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void Renderer::RenderCascadeShadow(VkCommandBuffer inCmd)
    {
        IsActiveCascade = m_SceneRenderer->HasDirectionLight();
        if (!IsActiveCascade)
            return;
        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();

        m_DirectLightPass->Render(inCmd, currentFrame.GlobalDescriptor, m_SceneRenderer->GetDrawContext(), m_SceneRenderer->GetGPUSceneData().Viewproj);
    }

    void Renderer::DrawSkybox(VkCommandBuffer inCmd)
    {
        PROFILE_FUNCTION();

        DrawContext skyboxContext;
        m_Skybox->Draw(skyboxContext);
        const RenderObject &object = skyboxContext.OpaqueSurfaces[0];

        vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_RenderPipeline->SkyboxPipline->Pipeline);

        vkCmdBindDescriptorSets(inCmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                m_RenderPipeline->SkyboxPipline->Layout,
                                0,
                                1,
                                &m_DeviceContext.GetCurrentFrame().GlobalDescriptor,
                                0,
                                nullptr);

        constexpr VkDeviceSize offsets[1] = {0};
        vkCmdBindVertexBuffers(inCmd, 0, 1, &object.MeshBuffer.VertexBuffer.Buffer, offsets);

        vkCmdBindIndexBuffer(inCmd, object.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

        const GPUDrawPushConstants push_constants{
                .WorldMatrix = glm::mat4(1.f), 
                .VertexBuffer = object.MeshBuffer.VertexBufferAddress,
        };

        constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
        vkCmdPushConstants(
                inCmd, m_RenderPipeline->SkyboxPipline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &push_constants);

        vkCmdDrawIndexed(inCmd, object.IndexCount, 1, object.FirstIndex, 0, 0);
    }

    void Renderer::DrawPickingPass(VkCommandBuffer cmd, const uint32_t inCurrentSwapchainIndex)
    {
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, cmd, "DrawPickingPass");

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 0.0f}};
        clearValues[1].depthStencil = {1.0f, 0};

        const VkRenderingAttachmentInfo colorAttachment =
                vkinit::attachment_info(m_DeviceContext.PickingImages[inCurrentSwapchainIndex].ImageView, clearValues.data());

        const VkRenderingAttachmentInfo depthAttachment =
                vkinit::depth_attachment_info(m_DeviceContext.DepthImage.ImageView, VK_ATTACHMENT_LOAD_OP_CLEAR, 0.f);
        const VkExtent2D extent = m_Window->GetExtent();

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);
        // vkCmdDispatch(cmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);
        vkCmdBeginRendering(cmd, &renderInfo);

        SetViewportScissor(cmd, m_Window->GetExtent());

        m_SceneRenderer->RenderPicking(cmd, m_DeviceContext.GetCurrentFrame().GlobalDescriptor, m_PickingPipeline->Pipeline, m_PickingPipeline->Layout);

        vkCmdEndRendering(cmd);
    }

    void Renderer::UpdateUniformBuffers()
    {
        PROFILE_ZONE("Update uniform buffers");

        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();

        // Cascade Matrix Buffer
        {
            std::array<glm::mat4, CASCADECOUNT> cascadeViewProjMatrices{};
            for (size_t i{0}; i < CASCADECOUNT; i++)
            {
                cascadeViewProjMatrices[i] = m_DirectLightPass->GetCascade(i).ViewProjectMatrix;
            }
            currentFrame.CascadeMatrixBuffer.Copy(cascadeViewProjMatrices.data());
        }

        // Scene Buffer
        {
            const GPUSceneData &sceneData = m_SceneRenderer->GetGPUSceneData();
            currentFrame.SceneDataBuffer.Copy(&sceneData);
        }

        // Cascade Data Buffer
        {
            const CascadeData &cascadeData = m_SceneRenderer->GetCascadeData();
            currentFrame.CascadeDataBuffer.Copy(&cascadeData);
        }

    }

    void Renderer::UpdateSceneRenderer(float inDeltaTime)
    {
        m_DirectLightPass->UpdateCascade(m_SceneRenderer->GetCamera(), m_SceneRenderer->GetGPUSceneData().LightDirection);
        m_SceneRenderer->Update(m_Window->GetExtent(), m_DirectLightPass->GetCascades(), inDeltaTime);
    }

    void Renderer::DestroyPipelines()
    {
        m_DeviceContext.RenderPipeline.reset();
        m_RenderPipeline.reset();
        m_PickingPipeline.reset();
    }

    uint64_t Renderer::TryPickObject(const glm::vec2 &inMousePos)
    {
        constexpr size_t bufferSize{sizeof(uint32_t) * 2};
        const auto      &extent = m_DeviceContext.Swapchain.GetExtent();

        const int32_t xPos = static_cast<int32_t>(inMousePos.x);
        const int32_t yPos = static_cast<int32_t>(extent.height - inMousePos.y);

        if (xPos >= extent.width || yPos >= extent.height)
        {
            return 0;
        }

        AllocatedBuffer stagingBuffer;
        stagingBuffer.Create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        auto &currentPickingImage = m_DeviceContext.PickingImages[m_DeviceContext.Swapchain.GetCurrentImageIndex()].Image;

        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkUtil::transition_image(cmd, currentPickingImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                    VkBufferImageCopy copyRegion               = {};
                    copyRegion.bufferOffset                    = 0;
                    copyRegion.bufferRowLength                 = 0;
                    copyRegion.bufferImageHeight               = 0;
                    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel       = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount     = 1;
                    copyRegion.imageOffset                     = {xPos, yPos, 0};
                    copyRegion.imageExtent                     = {1, 1, 1};

                    vkCmdCopyImageToBuffer(cmd, currentPickingImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer.Buffer, 1, &copyRegion);

                    VkUtil::transition_image(cmd, currentPickingImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                });

        uint32_t pickedData[2] = {};
        memcpy(&pickedData, stagingBuffer.Info.pMappedData, bufferSize);
        const uint64_t lower    = pickedData[0];
        const uint64_t upper    = pickedData[1];
        const uint64_t resultID = (upper << 32) | lower;

        stagingBuffer.Destroy();

        return resultID;
    }

    /// Shadows

    void Renderer::EnableCascade(bool inValue)
    {
        IsActiveCascade = inValue;

        Log::Info("Enable Cascade: {}", inValue);
    }

} // namespace MamontEngine