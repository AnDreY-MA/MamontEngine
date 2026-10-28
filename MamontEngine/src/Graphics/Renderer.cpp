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
#include "Graphics/Pass/PointLightPass.h"
#include "Graphics/Pass/PickPass.h"
#include "Graphics/DebugRenderer.h"
// #define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    Renderer::Renderer(VkContextDevice &inDeviceContext, const std::shared_ptr<WindowCore> &inWindow) 
        : m_DeviceContext(inDeviceContext), m_Window(inWindow)
    {
        m_SkyPass = std::make_unique<SkyPass>();


        const std::array<VkDescriptorSetLayout, 2> layouts = {m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

        m_DirectLightPass = std::make_unique<DirectLightPass>(m_DeviceContext.CascadeDepthImage.ImageFormat, m_DeviceContext.CascadeDepthImage.Image);
        m_DirectLightPass->CreatePipeline(layouts, m_DeviceContext.CascadeDepthImage.ImageFormat);

        m_PointLightPass = std::make_unique<PointLightPass>(m_DeviceContext.PointLightShadowMaps);

        m_PickPass = std::make_unique<PickPass>();
        //m_PickPass->CreatePipeline(layouts, m_DeviceContext.CascadeDepthImage.ImageFormat);
        InitPipelines();

        m_GpuHeapBudgets.resize(10000);
    }

    Renderer::~Renderer()
    {
        m_SceneRenderer.reset();
        m_SkyPass.reset();
        DestroyPipelines();
        DebugRenderer::Destroy();
    }

    void Renderer::InitSceneRenderer(const std::shared_ptr<Camera> &inMainCamera, const std::shared_ptr<Scene> &inScene)
    {
        m_SceneRenderer = std::make_shared<SceneRenderer>(inMainCamera, inScene);
    }

    void Renderer::InitImGuiRenderer(const std::shared_ptr<ImGuiLayer>& inLayer)
    {
        m_ImGuiRenderer = std::make_unique<ImGuiRenderer>(m_DeviceContext, m_Window->GetWindow(), m_DeviceContext.Swapchain.GetImageFormat(), inLayer);
    }

    void Renderer::InitPipelines()
    {
        const VkDevice device = LogicalDevice::GetDevice();
        vkDeviceWaitIdle(device);

        const std::pair<VkFormat, VkFormat> inImageFormats{m_DeviceContext.DrawImage.ImageFormat, m_DeviceContext.DepthImage.ImageFormat};

        fmt::println("DrawImage.ImageFormat: {}", string_VkFormat(inImageFormats.first));
        fmt::println("DepthImage.ImageFormat: {}", string_VkFormat(inImageFormats.second));

        const std::array<VkDescriptorSetLayout, 2> layouts{m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderDescriptorLayout};

        if (m_RenderPipeline)
        {
            m_RenderPipeline.reset();
        }
        
        m_RenderPipeline = std::make_shared<RenderPipeline>(device, layouts, inImageFormats);

        m_SkyPass->CreatePipeline(layouts, inImageFormats);
        m_PointLightPass->CreatePipeline(layouts, m_DeviceContext.CascadeDepthImage.ImageFormat);

        //InitPickPipepline();

        DebugRenderer::Init(device, layouts);

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
        VkUtil::transition_image(
                cmd, m_DeviceContext.DepthImage.Image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        
        if (const auto &lightData = m_SceneRenderer->GetLightData(); 
            lightData.HasDirectionLight || lightData.PointLightingCount > 0)
        {
            VkCommandBuffer shadowCommandBuffer = currentFrame.ShadowCommandBuffer;

            RenderShadows(shadowCommandBuffer);
            //secondaryCommandBuffers.push_back(shadowCommandBuffer);
            vkCmdExecuteCommands(cmd, 1, &shadowCommandBuffer);
        }

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkUtil::transition_image(
                cmd, m_DeviceContext.DepthImage.Image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

        VkImage currentSwapchainImage = m_DeviceContext.Swapchain.GetImageAt(swapchainImageIndex);
        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        //VkUtil::copy_image_to_image(cmd, m_DeviceContext.DrawImage.Image, currentSwapchainImage, m_DrawExtent, m_DeviceContext.Swapchain.GetExtent());

        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        // ImGui

        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        VkCommandBuffer uiCommandBuffer = currentFrame.UICommandBuffer;
        m_ImGuiRenderer->Draw(uiCommandBuffer, m_DeviceContext.Swapchain.GetImageView(swapchainImageIndex), m_DrawExtent);


        // End ImGui

        vkCmdExecuteCommands(cmd, 1, &uiCommandBuffer);
        VkUtil::transition_image(cmd, m_DeviceContext.DrawImage.Image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


        VK_CHECK(vkEndCommandBuffer(cmd));

        m_DeviceContext.EndFrame(cmd);
    }

    void Renderer::DrawMain(VkCommandBuffer inCmd)
    {
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "Draw Main");

        const auto start = std::chrono::high_resolution_clock::now();

        DrawGeometry(inCmd);

        const auto end     = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        m_Stats.MeshDrawTime = elapsed.count() / 1000.f;
    }

    void Renderer::DrawGeometry(VkCommandBuffer inCmd)
    {
        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();
        PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Draw Geometry");

        const VkExtent2D &extent = m_Window->GetExtent();

        constexpr VkClearValue                    clearColor      = {.color = {0.0f, 0.0f, 0.0f, 0.0f}};
        constexpr VkClearValue           clearDepth      = {.depthStencil = {1.0f, 0}};
        const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(m_DeviceContext.DrawImage.ImageView, &clearColor);
        const VkRenderingAttachmentInfo       depthAttachment = vkinit::depth_attachment_info(m_DeviceContext.DepthImage.ImageView, VK_ATTACHMENT_LOAD_OP_CLEAR, 1.f);
        //depthAttachment.clearValue                   = clearDepth;

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(inCmd, &renderInfo);

        SetViewportScissor(inCmd, m_DrawExtent);
        vkCmdSetDepthBias(inCmd, 0, 0, 0);

        m_SkyPass->Render(inCmd, currentFrame.GlobalDescriptor);

        m_SceneRenderer->Render(inCmd, currentFrame.GlobalDescriptor, m_RenderPipeline.get());

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

    void Renderer::RenderShadows(VkCommandBuffer inCmd)
    {
        const auto start = std::chrono::high_resolution_clock::now();

        RenderCascadeShadow(inCmd);

        const auto &lightData = m_SceneRenderer->GetLightData();
        if (lightData.PointLightingCount > 0)
        {
            m_PointLightPass->Render(
                    inCmd, m_DeviceContext.GetCurrentFrame().GlobalDescriptor, m_SceneRenderer->GetDrawContext(), m_SceneRenderer->GetGPUSceneData().Viewproj);
        }

        const auto end     = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        m_Stats.ShadowsDrawTime = elapsed.count() / 1000.f;

    }

    void Renderer::RenderCascadeShadow(VkCommandBuffer inCmd)
    {
        if (!m_SceneRenderer->HasDirectionLight())
            return;

        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();
        m_DirectLightPass->Render(inCmd, currentFrame.GlobalDescriptor, m_SceneRenderer->GetDrawContext(), m_SceneRenderer->GetGPUSceneData().Viewproj);
    }

    void Renderer::UpdateUniformBuffers()
    {
        PROFILE_ZONE("Update uniform buffers");

        const auto &currentFrame = m_DeviceContext.GetCurrentFrame();

        // Scene Buffer
        {
            const GPUSceneData &sceneData = m_SceneRenderer->GetGPUSceneData();
            currentFrame.SceneDataBuffer.Copy(&sceneData);
        }

        // Cascade Matrix Buffer
        {
            currentFrame.CascadeMatrixBuffer.Copy(&m_DirectLightPass->GetCascades());
        }
        

        // Light Data Buffer
        {
            const LightData &lightData = m_SceneRenderer->GetLightData();
            currentFrame.LightDataBuffer.Copy(&lightData);
        }

    }

    const MemoryStats &Renderer::GetGPUMemoryStats() const
    {
        VmaBudget budget{};
        Allocator::GetHeapBudgets(&budget);
        MemoryStats gpuMemoryStats;
        gpuMemoryStats.MemoryUsed = budget.usage;
        gpuMemoryStats.MemoryAllocated = budget.budget;

        /*for (uint32_t i = 0; i < Allocator::GetHeapCount(); ++i)
        {
            gpuMemoryStats.MemoryUsed += m_GpuHeapBudgets[i].usage;
            gpuMemoryStats.MemoryAllocated += m_GpuHeapBudgets[i].budget;
        }*/

        return gpuMemoryStats;
    }

    void Renderer::UpdateSceneRenderer(float inDeltaTime)
    {
        m_SceneRenderer->Update(m_Window->GetExtent(), inDeltaTime);

        m_DirectLightPass->UpdateCascade(m_SceneRenderer->GetCamera(), m_SceneRenderer->GetLightData().DirectLight.Direction);
        m_PointLightPass->UpdateLights(m_SceneRenderer->GetCamera(), m_SceneRenderer->GetLightData());
    }

    void Renderer::DestroyPipelines()
    {
        m_RenderPipeline.reset();
    }   

} // namespace MamontEngine