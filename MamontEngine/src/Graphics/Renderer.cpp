#include "Renderer.h"
#include "Core/ContextDevice.h"

#include "Core/VkInitializers.h"
#include "Core/VkImages.h"
#include "Core/VkPipelines.h"
#include "Core/Window.h"
#include "Core/ImGuiRenderer.h"

#include <chrono>
#include <thread>
#include "Utils/Profile.h"

//#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MamontEngine
{
    Renderer::Renderer(VkContextDevice &inDeviceContext, const std::shared_ptr<WindowCore>& inWindow) 
        : m_DeviceContext(inDeviceContext), m_Window(inWindow)
    {

    }

    void Renderer::Clear()
    {
        m_SceneRenderer->Clear();
    }

    void Renderer::InitSceneRenderer(const std::shared_ptr<Camera> &inMainCamera)
    {
        m_SceneRenderer = std::make_shared<SceneRenderer>(inMainCamera);
    }
    
    void Renderer::InitImGuiRenderer(VkDescriptorPool &outDescPool)
    {
        m_ImGuiRenderer = std::make_unique<ImGuiRenderer>(
            m_DeviceContext, m_Window->GetWindow(), m_DeviceContext.Swapchain.GetImageFormat(), outDescPool);
    }

    void Renderer::InitPipelines()
    {
        const std::pair<VkFormat, VkFormat> inImageFormats{m_DeviceContext.Image.DrawImage.ImageFormat, m_DeviceContext.Image.DepthImage.ImageFormat};
        m_RenderPipeline = std::make_unique<RenderPipeline>(m_DeviceContext.Device,
                                                            m_DeviceContext.GPUSceneDataDescriptorLayout, inImageFormats);
        m_SkyPipeline    = std::make_unique<SkyPipeline>(m_DeviceContext.Device, &m_DeviceContext.DrawImageDescriptorLayout);

        m_DeviceContext.RenderPipeline = m_RenderPipeline.get();
    }

    void Renderer::Render()
    {
        PROFILE_ZONE("Renderer::Render");

        if (vkGetFenceStatus(m_DeviceContext.Device, m_DeviceContext.GetCurrentFrame().RenderFence) != VK_SUCCESS)
        {
            VK_CHECK_MESSAGE(vkWaitForFences(m_DeviceContext.Device, 1, &m_DeviceContext.GetCurrentFrame().RenderFence, VK_TRUE, 1000000000), "Wait FENCE");
        }

        m_DeviceContext.GetCurrentFrame().Deleteions.Flush();
        m_DeviceContext.GetCurrentFrame().FrameDescriptors.ClearPools(m_DeviceContext.Device);

        const auto [e, swapchainImageIndex] =
                m_DeviceContext.Swapchain.AcquireImage(m_DeviceContext.Device, m_DeviceContext.GetCurrentFrame().SwapchainSemaphore);
        if (e == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_IsResizeRequested = true;
            ResizeSwapchain();
            return;
        }

        const VkExtent2D swapchainExtent = m_DeviceContext.Swapchain.GetExtent();
        m_DrawExtent.height              = std::min(swapchainExtent.height, m_DeviceContext.Image.DrawImage.ImageExtent.height) * m_RenderScale;
        m_DrawExtent.width               = std::min(swapchainExtent.width, m_DeviceContext.Image.DrawImage.ImageExtent.width) * m_RenderScale;

        VK_CHECK(vkResetFences(m_DeviceContext.Device, 1, &m_DeviceContext.GetCurrentFrame().RenderFence));
        VK_CHECK(vkResetCommandBuffer(m_DeviceContext.GetCurrentFrame().MainCommandBuffer, 0));

        const VkCommandBuffer cmd = m_DeviceContext.GetCurrentFrame().MainCommandBuffer;

        const VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        VkUtil::transition_image(cmd, m_DeviceContext.Image.DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkUtil::transition_image(
                cmd, m_DeviceContext.Swapchain.GetImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        //< draw_first

        //> imgui_draw

        VkUtil::copy_image_to_image(cmd,
                                    m_DeviceContext.Image.DrawImage.Image,
                                    m_DeviceContext.Swapchain.GetImages()[swapchainImageIndex],
                                    m_DrawExtent,
                                    m_DeviceContext.Swapchain.GetExtent());

        VkUtil::transition_image(cmd,
                                 m_DeviceContext.Swapchain.GetImages()[swapchainImageIndex],
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        DrawImGui(cmd, m_DeviceContext.Swapchain.GetImageView(swapchainImageIndex));


        VkUtil::transition_image(
                cmd, m_DeviceContext.Swapchain.GetImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        // vkCmdEndRenderPass(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));
        //< imgui_draw

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        const VkSemaphoreSubmitInfo     waitInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, m_DeviceContext.GetCurrentFrame().SwapchainSemaphore);
        const VkSemaphoreSubmitInfo signalInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_DeviceContext.GetCurrentFrame().RenderSemaphore);

        const VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(m_DeviceContext.GetGraphicsQueue(), 1, &submit, m_DeviceContext.GetCurrentFrame().RenderFence));


        const VkResult presentResult =
                m_DeviceContext.Swapchain.Present(m_DeviceContext.GetGraphicsQueue(), &m_DeviceContext.GetCurrentFrame().RenderSemaphore, swapchainImageIndex);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_IsResizeRequested = true;
            return;
        }

        m_DeviceContext.IncrementFrameNumber();
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

    void Renderer::UpdateSceneRenderer()
    {
        m_SceneRenderer->Update(m_Window->GetExtent());
    }

    void Renderer::DestroyPipelines()
    {
        m_RenderPipeline->Destroy(m_DeviceContext.Device);
        m_SkyPipeline->Destroy(m_DeviceContext.Device);
    }

    void Renderer::DrawMain(VkCommandBuffer inCmd)
    {
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "DrawMain");

        {
            PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "Draw SkyPipeline");
            m_SkyPipeline->Draw(inCmd, &m_DeviceContext.DrawImageDescriptors);
        }

        const VkExtent2D extent = m_Window->GetExtent();
        vkCmdDispatch(inCmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);

        VkUtil::transition_image(inCmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        const VkRenderingAttachmentInfo colorAttachment =
                vkinit::attachment_info(m_DeviceContext.Image.DrawImage.ImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        const VkRenderingAttachmentInfo depthAttachment =
                vkinit::depth_attachment_info(m_DeviceContext.Image.DepthImage.ImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(inCmd, &renderInfo);

        const auto start = std::chrono::high_resolution_clock::now();

        DrawGeometry(inCmd);

        const auto end     = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        m_Stats.MeshDrawTime = elapsed.count() / 1000.f;

        vkCmdEndRendering(inCmd);
    }

    void Renderer::DrawGeometry(VkCommandBuffer inCmd)
    {
        //PROFILE_ZONE("Renderer::DrawGeometry");
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "Draw Geometry");

        auto &SceneBuffer = m_DeviceContext.GetCurrentFrame().SceneDataBuffer;
        if (SceneBuffer.MappedData == nullptr)
        {
            fmt::println("GPUSceneData buffer mapping returned nullptr");
            return;
        }

        static_assert(std::is_standard_layout<GPUSceneData>::value, "GPUSceneData must be standard layout");
        static_assert(std::is_trivially_copyable<GPUSceneData>::value, "GPUSceneData must be trivially copyable");

        const GPUSceneData &sceneData = m_SceneRenderer->GetGPUSceneData();
        std::memcpy(SceneBuffer.MappedData, &sceneData, sizeof(GPUSceneData));

        VkDescriptorSet globalDescriptor = m_DeviceContext.GetCurrentFrame().FrameDescriptors.Allocate(
                m_DeviceContext.Device, m_DeviceContext.GPUSceneDataDescriptorLayout /*, &allocArrayInfo*/);

        {
            DescriptorWriter writer;
            writer.WriteBuffer(0, SceneBuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.UpdateSet(m_DeviceContext.Device, globalDescriptor);
        }

        {
            const VkViewport viewport = {0, 0, (float)m_DrawExtent.width, (float)m_DrawExtent.height, 0.f, 1.f};
            const VkRect2D   scissor  = {0, 0, m_DrawExtent};

            vkCmdSetViewport(inCmd, 0, 1, &viewport);
            vkCmdSetScissor(inCmd, 0, 1, &scissor);
        }

        {
            PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "Scene Render");
            m_SceneRenderer->Render(inCmd, globalDescriptor, sceneData);
        }

        /*stats.DrawCallCount = 0;
        stats.TriangleCount = 0;*/
    }

    void Renderer::DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView)
    {
        m_ImGuiRenderer->Draw(inCmd, inTargetImageView, m_DeviceContext.Swapchain.GetExtent());
    }

    void Renderer::ResizeSwapchain()
    {
        m_IsResizeRequested = true;
        if (!m_IsResizeRequested)
            return;

        fmt::println("Resize Swapchain");

        m_DeviceContext.ResizeSwapchain(m_Window->Resize());

        m_IsResizeRequested = false;
    }

} // namespace MamontEngine
