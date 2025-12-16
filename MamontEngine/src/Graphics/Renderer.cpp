#include "Renderer.h"
#include "Core/ContextDevice.h"

#include "Core/ImGuiRenderer.h"
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

#include "Core/Log.h"
#include "Graphics/Devices/LogicalDevice.h"
// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    Renderer::Renderer(VkContextDevice &inDeviceContext, const std::shared_ptr<WindowCore> &inWindow) : m_DeviceContext(inDeviceContext), m_Window(inWindow)
    {
    }

    void Renderer::Clear()
    {
        m_SceneRenderer->Clear();
    }

    void Renderer::InitSceneRenderer(const std::shared_ptr<Camera> &inMainCamera)
    {
        m_SceneRenderer = std::make_shared<SceneRenderer>(inMainCamera);
        const std::string cubePath = DEFAULT_ASSETS_DIRECTORY + "cube.glb";
        m_Skybox                   = std::make_unique<MeshModel>(m_DeviceContext, 0, cubePath);

        VkDeviceAddress vertexAddress{0};
        {
            DrawContext skyboxContext;
            m_Skybox->Draw(skyboxContext);
            const RenderObject &object = skyboxContext.OpaqueSurfaces[0];
            vertexAddress = object.VertexBufferAddress;
        }

        m_DeviceContext.CreatePrefilteredCubeTexture(vertexAddress, [&](VkCommandBuffer cmd) {
                    DrawContext skyboxContext;
                    m_Skybox->Draw(skyboxContext);
                    const RenderObject &object = skyboxContext.OpaqueSurfaces[0];

                    constexpr VkDeviceSize offsets[1] = {0};
                    vkCmdBindVertexBuffers(cmd, 0, 1, &object.VertexBuffer, offsets);

                    vkCmdBindIndexBuffer(cmd, object.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

        const std::pair<VkFormat, VkFormat> inImageFormats{m_DeviceContext.Image.DrawImage.ImageFormat, m_DeviceContext.Image.DepthImage.ImageFormat};

        fmt::println("DrawImage.ImageFormat: {}", string_VkFormat(inImageFormats.first));
        fmt::println("DepthImage.ImageFormat: {}", string_VkFormat(inImageFormats.second));

        std::array<VkDescriptorSetLayout, 2> laouts{m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

        m_RenderPipeline = std::make_unique<RenderPipeline>(device, laouts, inImageFormats);

        m_DeviceContext.RenderPipeline = m_RenderPipeline.get();

        InitPickPipepline();

        CreateShadowPipeline();

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
        pipelineBuilder.SetDepthFormat(m_DeviceContext.Image.DepthImage.ImageFormat);

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

        auto          &currentFrame = m_DeviceContext.GetCurrentFrame();
        const VkDevice device       = LogicalDevice::GetDevice();

        /*if (vkGetFenceStatus(device, currentFrame.RenderFence) != VK_SUCCESS)
        {

        }*/
        VK_CHECK_MESSAGE(vkWaitForFences(device, 1, &currentFrame.RenderFence, VK_TRUE, UINT64_MAX), "Wait FENCE");
        VK_CHECK(vkResetFences(device, 1, &currentFrame.RenderFence));

        currentFrame.Deleteions.Flush();

        const auto [resultAcquire, swapchainImageIndex] = m_DeviceContext.Swapchain.AcquireImage(device, currentFrame.SwapchainSemaphore);
        if (resultAcquire == VK_ERROR_OUT_OF_DATE_KHR || resultAcquire == VK_SUBOPTIMAL_KHR)
        {
            m_IsResizeRequested = true;
            // ResizeSwapchain();
            return;
        }

        const VkExtent2D swapchainExtent = m_DeviceContext.Swapchain.GetExtent();
        m_DrawExtent                     = swapchainExtent;

        VkCommandBuffer cmd = currentFrame.MainCommandBuffer;

        VK_CHECK(vkResetCommandBuffer(cmd, 0));

        const VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkUtil::transition_image(cmd, m_DeviceContext.Image.DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        VkImage currentSwapchainImage = m_DeviceContext.Swapchain.GetImageAt(swapchainImageIndex);
        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkUtil::copy_image_to_image(cmd, m_DeviceContext.Image.DrawImage.Image, currentSwapchainImage, m_DrawExtent, m_DeviceContext.Swapchain.GetExtent());

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        DrawImGui(cmd, m_DeviceContext.Swapchain.GetImageView(swapchainImageIndex));

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

        m_SceneRenderer->ClearDrawContext();

        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        const VkSemaphoreSubmitInfo     waitInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.SwapchainSemaphore);
        const VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(0, m_DeviceContext.RenderCopleteSemaphores[swapchainImageIndex]);

        const VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);
        VK_CHECK(vkQueueSubmit2(m_DeviceContext.GetGraphicsQueue(), 1, &submit, currentFrame.RenderFence));

        const VkResult presentResult = m_DeviceContext.Swapchain.Present(
                m_DeviceContext.GetGraphicsQueue(), &m_DeviceContext.RenderCopleteSemaphores[swapchainImageIndex], swapchainImageIndex);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            m_IsResizeRequested = true;
            //ResizeSwapchain();
            return;
        }

        m_DeviceContext.IncrementFrameNumber();
    }

    void Renderer::DrawMain(VkCommandBuffer inCmd)
    {
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "DrawMain");

        UpdateUniformBuffers();

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

        /*VkUtil::transition_image(
                inCmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkUtil::transition_image(
                inCmd, m_DeviceContext.Image.DepthImage.Image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);*/
        VkClearValue                    clearColor      = {.color = {0.0f, 0.0f, 0.0f, 0.0f}};
        VkClearValue                    clearDepth      = {.depthStencil = {1.0f, 0}};
        const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(m_DeviceContext.Image.DrawImage.ImageView, &clearColor);
        VkRenderingAttachmentInfo       depthAttachment = vkinit::depth_attachment_info(m_DeviceContext.Image.DepthImage.ImageView);
        depthAttachment.clearValue.depthStencil         = {1.f, 0};

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(inCmd, &renderInfo);

        SetViewportScissor(inCmd, m_DrawExtent);
        vkCmdSetDepthBias(inCmd, 0, 0, 0);

        DrawSkybox(inCmd);

        {
            PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Scene Render");
            m_SceneRenderer->Render(inCmd, currentFrame.GlobalDescriptor);
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
        if (!IsActiveCascade)
            return;
        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();
        PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Render Cascade Shadow");

        constexpr VkExtent2D cascadeExtent = {.width = SHADOWMAP_DIMENSION, .height = SHADOWMAP_DIMENSION};

        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            VkUtil::transition_image(inCmd, m_DeviceContext.CascadeDepthImage.Image.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

            const VkImageView cascadeDepth = m_DeviceContext.Cascades[i].View;
            if (cascadeDepth == VK_NULL_HANDLE)
            {
                fmt::println("cascadeDepth is Null");
                continue;
            }
            VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(cascadeDepth);
            depthAttachment.clearValue.depthStencil   = {1.f, 0};
            depthAttachment.loadOp                    = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp                   = VK_ATTACHMENT_STORE_OP_STORE;

            const VkRenderingInfo renderCascadeInfo = vkinit::rendering_info(cascadeExtent, nullptr, &depthAttachment);

            vkCmdBeginRendering(inCmd, &renderCascadeInfo);
            vkCmdSetDepthBias(inCmd, 1.25f, 0.f, 1.75f);
            SetViewportScissor(inCmd, cascadeExtent);
            vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CascadePipeline->Pipeline);

            m_SceneRenderer->RenderShadow(
                    inCmd, currentFrame.GlobalDescriptor, m_DeviceContext.Cascades[i].ViewProjectMatrix, *m_CascadePipeline, static_cast<uint32_t>(i));

            vkCmdEndRendering(inCmd);
            VkUtil::transition_image(inCmd,
                                     m_DeviceContext.CascadeDepthImage.Image.Image,
                                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        }
    }

    void Renderer::DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView)
    {
        m_ImGuiRenderer->Draw(inCmd, inTargetImageView, m_DeviceContext.Swapchain.GetExtent());
    }

    void Renderer::DrawSkybox(VkCommandBuffer inCmd)
    {
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
        vkCmdBindVertexBuffers(inCmd, 0, 1, &object.VertexBuffer, offsets);

        vkCmdBindIndexBuffer(inCmd, object.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

        const GPUDrawPushConstants push_constants{
                .WorldMatrix = glm::mat4(1.f), 
                .VertexBuffer = object.VertexBufferAddress,
        };

        constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
        vkCmdPushConstants(
                inCmd, object.Material->Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &push_constants);

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
                vkinit::depth_attachment_info(m_DeviceContext.Image.DepthImage.ImageView, VK_ATTACHMENT_LOAD_OP_CLEAR, 0.f);
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
        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();

        // Cascade Matrix Buffer
        {
            std::array<glm::mat4, CASCADECOUNT> cascadeViewProjMatrices{};
            for (size_t i{0}; i < CASCADECOUNT; i++)
            {
                cascadeViewProjMatrices[i] = m_DeviceContext.Cascades[i].ViewProjectMatrix;
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

    void Renderer::ResizeSwapchain()
    {
        if (!m_IsResizeRequested)
            return;

        fmt::println("Resize Swapchain");

        m_DeviceContext.ResizeSwapchain(m_Window->Resize());

        m_IsResizeRequested = false;
    }

    void Renderer::UpdateWindowEvent(const SDL_EventType inType)
    {
        if (inType == SDL_EVENT_WINDOW_RESIZED)
        {
            m_IsResizeRequested = true;
        }

        if (inType == SDL_EVENT_WINDOW_MINIMIZED)
        {
            m_StopRendering = true;
        }
        if (inType == SDL_EVENT_WINDOW_RESTORED)
        {
            m_StopRendering = false;
        }
    }

    void Renderer::UpdateSceneRenderer(float inDeltaTime)
    {
        m_SceneRenderer->UpdateCascades(m_DeviceContext.Cascades);

        m_SceneRenderer->Update(m_Window->GetExtent(), m_DeviceContext.Cascades, inDeltaTime);
    }

    void Renderer::DestroyPipelines()
    {
        m_RenderPipeline.release();
        m_PickingPipeline.release();
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


    void Renderer::CreateShadowPipeline()
    {
        constexpr VkPushConstantRange matrixRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};
        const VkDevice device = LogicalDevice::GetDevice();

        const std::array<VkDescriptorSetLayout, 2> layouts = {m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

        const VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info(static_cast<uint32_t>(layouts.size()), layouts.data(), &matrixRange);

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
        pipelineBuilder.SetDepthFormat(m_DeviceContext.CascadeDepthImage.Image.ImageFormat);
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

    void Renderer::EnableCascade(bool inValue)
    {
        IsActiveCascade = inValue;

        Log::Info("Enable Cascade: {}", inValue);
    }

} // namespace MamontEngine
////////////////