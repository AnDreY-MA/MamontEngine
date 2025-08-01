#include "Engine.h"

#include "VkInitializers.h"
#include "VkImages.h"
#include "VkPipelines.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_events.h>

#include <chrono>
#include <thread>
#include <VkBootstrap.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include <glm/gtx/transform.hpp>
#include "Graphics/Vulkan/Swapchain.h"
#include "ImGuiRenderer.h"

#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include <ECS/Components/MeshComponent.h>

#include "ImGuizmo/ImGuizmo.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>


namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;
    MEngine       *loadedEngine         = nullptr;
    MEngine       &MEngine::Get()
    {
        return *loadedEngine;
    }

    const std::string RootDirectories = PROJECT_ROOT_DIR;

    void MEngine::Init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        m_Window = std::make_shared<WindowCore>();
        //m_Window->Init();

        m_ContextDevice = std::make_unique<VkContextDevice>();

        m_Renderer = Renderer::CreateRenderer();

        InitVulkan();
        
        InitSwapchain();

        CreateRenderPass();
        
        InitCommands();

        InitFrameBuffers();

        m_ImGuiRenderer.CreateViewportImage(m_ContextDevice->Device, m_Swapchain.GetImages().size(), m_Swapchain.GetExtent());
        m_ImGuiRenderer.CreateViewportImageViews(m_ContextDevice->Device);
        
        InitSyncStructeres();
        
        InitDescriptors();

        InitPipelines();    

        InitImgui();

        InitDefaultData();

        InitScene();

        //InitImgui();

        m_MainCamera->SetVelocity(glm::vec3(0.f));
        m_MainCamera->SetPosition(glm::vec3(1, 1, 0));
        m_IsInitialized = true;
    }

    void MEngine::Run()
    {
        SDL_Event event;
        bool      bQuit{false};

        while (!bQuit)
        {
            const auto start = std::chrono::high_resolution_clock::now();

            while (SDL_PollEvent(&event) != 0)
            { 
                if (event.type == SDL_EVENT_QUIT)
                    bQuit = true;

                if (event.window.type == SDL_EVENT_WINDOW_RESIZED)
                {
                    m_IsResizeRequested = true;
                }

                if (event.window.type == SDL_EVENT_WINDOW_MINIMIZED)
                {
                    m_StopRendering = true;
                }
                if (event.window.type == SDL_EVENT_WINDOW_RESTORED)
                {
                    m_StopRendering = false;
                }

                m_MainCamera->ProccessEvent(event);

                ImGui_ImplSDL3_ProcessEvent(&event);
            }

            if (m_StopRendering)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (m_IsResizeRequested)
            {
                fmt::println("Resize Swapchain");
                ResizeSwapchain();
            }

            if (!m_IsResizeRequested)
            {
                m_GuiLayer->Begin();

                m_GuiLayer->ImGuiRender();

                m_GuiLayer->End();

                UpdateScene();
                Draw();
            }

            const auto end     = std::chrono::high_resolution_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            stats.FrameTime = elapsed.count() / 1000.f;
        }
    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            vkDeviceWaitIdle(m_ContextDevice->Device);
            
            m_GuiLayer->Deactivate();
            
            m_SceneRenderer->Clear();
            
            m_MainDeletionQueue.Flush();

            m_Swapchain.Destroy(m_ContextDevice->Device);

            //~ContextDevice

            //m_Window->Close();
        }

        loadedEngine = nullptr;
    }

    void MEngine::InitDefaultData()
    {
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(m_ContextDevice->Device, &sampl, nullptr, &m_ContextDevice->DefaultSamplerNearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;

        vkCreateSampler(m_ContextDevice->Device, &sampl, nullptr, &m_ContextDevice->DefaultSamplerLinear);

        const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        m_ContextDevice->WhiteImage =
                m_ContextDevice->CreateImage((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

        const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));

        const uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }
        m_ContextDevice->ErrorCheckerboardImage =
                m_ContextDevice->CreateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);


        m_MainDeletionQueue.PushFunction(
                [&]()
                {
                    vkDestroySampler(m_ContextDevice->Device, m_ContextDevice->DefaultSamplerNearest, nullptr);
                    vkDestroySampler(m_ContextDevice->Device, m_ContextDevice->DefaultSamplerLinear, nullptr);

                    m_ContextDevice->DestroyImage(m_ContextDevice->WhiteImage);
                    m_ContextDevice->DestroyImage(m_ContextDevice->ErrorCheckerboardImage);
                });
    }
    
    void MEngine::Draw()
    {
        if (vkGetFenceStatus(m_ContextDevice->Device, m_ContextDevice->GetCurrentFrame().RenderFence) != VK_SUCCESS)
        {
            VK_CHECK_MESSAGE(vkWaitForFences(m_ContextDevice->Device, 1, &m_ContextDevice->GetCurrentFrame().RenderFence, VK_TRUE, 1000000000), "Wait FENCE");
        }

        m_ContextDevice->GetCurrentFrame().Deleteions.Flush();
        m_ContextDevice->GetCurrentFrame().FrameDescriptors.ClearPools(m_ContextDevice->Device);

        const auto [e, swapchainImageIndex] = m_Swapchain.AcquireImage(m_ContextDevice->Device, m_ContextDevice->GetCurrentFrame() .SwapchainSemaphore);
        if (e == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_IsResizeRequested = true;
            ResizeSwapchain();
            return;
        } 

        const VkExtent2D swapchainExtent = m_Swapchain.GetExtent();
        m_DrawExtent.height              = std::min(swapchainExtent.height, m_ContextDevice->Image.DrawImage.ImageExtent.height) * m_RenderScale;
        m_DrawExtent.width               = std::min(swapchainExtent.width, m_ContextDevice->Image.DrawImage.ImageExtent.width) * m_RenderScale;

        VK_CHECK(vkResetFences(m_ContextDevice->Device, 1, &m_ContextDevice->GetCurrentFrame().RenderFence));
        VK_CHECK(vkResetCommandBuffer(m_ContextDevice->GetCurrentFrame().MainCommandBuffer, 0));
          
        VkCommandBuffer cmd = m_ContextDevice->GetCurrentFrame().MainCommandBuffer;
        
        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_ContextDevice->Image.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        VkUtil::transition_image(cmd, m_ContextDevice->Image.DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_ContextDevice->Image.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkUtil::transition_image(cmd, m_Swapchain.GetImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        //< draw_first
    
        //> imgui_draw

        VkUtil::copy_image_to_image(
                cmd, m_ContextDevice->Image.DrawImage.Image, m_Swapchain.GetImages()[swapchainImageIndex], m_DrawExtent, m_Swapchain.GetExtent());

        VkUtil::transition_image(
                cmd, m_Swapchain.GetImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        DrawImGui(cmd, m_Swapchain.GetImageView(swapchainImageIndex));

        VkUtil::transition_image(cmd, m_Swapchain.GetImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(cmd));
        //< imgui_draw

        VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        VkSemaphoreSubmitInfo     waitInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, m_ContextDevice->GetCurrentFrame() .SwapchainSemaphore);
        VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_ContextDevice->GetCurrentFrame() .RenderSemaphore);

        VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(m_ContextDevice->GraphicsQueue, 1, &submit, m_ContextDevice->GetCurrentFrame().RenderFence));

        const VkResult presentResult = m_Swapchain.Present(m_ContextDevice->GraphicsQueue, &m_ContextDevice->GetCurrentFrame().RenderSemaphore, swapchainImageIndex);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_IsResizeRequested = true;
            return;
        }

        m_ContextDevice->IncrementFrameNumber();
    }

    void MEngine::DrawMain(VkCommandBuffer inCmd)
    {
        m_SkyPipeline->Draw(inCmd, &m_DrawImageDescriptors);

        const VkExtent2D extent = m_Window->GetExtent();
        vkCmdDispatch(inCmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);

        VkUtil::transition_image(inCmd, m_ContextDevice->Image.DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo colorAttachment =
                vkinit::attachment_info(m_ContextDevice->Image.DrawImage.ImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo depthAttachment =
                vkinit::depth_attachment_info(m_ContextDevice->Image.DepthImage.ImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(inCmd, &renderInfo);

        const auto start = std::chrono::high_resolution_clock::now();

        DrawGeometry(inCmd);

        const auto end     = std::chrono::high_resolution_clock::now();
        const auto  elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        stats.MeshDrawTime = elapsed.count() / 1000.f;

        vkCmdEndRendering(inCmd);
    }

    void MEngine::DrawGeometry(VkCommandBuffer inCmd)
    {
        const AllocatedBuffer gpuSceneDataBuffer =
                m_ContextDevice->CreateBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        m_ContextDevice->GetCurrentFrame().Deleteions.PushFunction([=, this]() { 
            m_ContextDevice->DestroyBuffer(gpuSceneDataBuffer); 
            });

        // write the buffer
        GPUSceneData *sceneUniformData = (GPUSceneData *)gpuSceneDataBuffer.Allocation->GetMappedData();
        *sceneUniformData              = m_SceneRenderer->GetGPUSceneData();

        const VkDescriptorSet& globalDescriptor = m_ContextDevice->GetCurrentFrame().FrameDescriptors.Allocate(m_ContextDevice->Device, m_GPUSceneDataDescriptorLayout/*, &allocArrayInfo*/);

        DescriptorWriter writer;
        writer.WriteBuffer(0, gpuSceneDataBuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.UpdateSet(m_ContextDevice->Device, globalDescriptor);

        m_RenderPipeline->Draw(inCmd, globalDescriptor, m_SceneRenderer->GetGPUSceneData(), m_DrawExtent);

        stats.DrawCallCount = 0;
        stats.TriangleCount = 0;

    }

    void MEngine::DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView)
    {
        m_ImGuiRenderer.Draw(inCmd, inTargetImageView, m_Swapchain.GetExtent());
    }
    
    void MEngine::InitScene()
    {
        m_MainCamera    = std::make_shared<Camera>();
        m_SceneRenderer = std::make_shared<SceneRenderer>(m_MainCamera, m_RenderPipeline);
        m_Scene = std::make_shared<Scene>(m_SceneRenderer);
        m_Scene->Init(*m_ContextDevice);
    }

    void MEngine::UpdateScene()
    {
        m_MainCamera->Update();
        m_Scene->Update();
        m_SceneRenderer->Update(m_Window->GetExtent());

    }
    
    void MEngine::InitVulkan()
    {
        m_ContextDevice->Init(m_Window.get());

    }
    
    void MEngine::InitSwapchain()
    {
        m_Swapchain.Init(*m_ContextDevice, m_Window->GetExtent(), m_ContextDevice->Image);

        m_MainDeletionQueue.PushFunction([=]() { 
                    vkDestroyImageView(m_ContextDevice->Device, m_ContextDevice->Image.DrawImage.ImageView, nullptr);
                    vmaDestroyImage(m_ContextDevice->Allocator, m_ContextDevice->Image.DrawImage.Image, m_ContextDevice->Image.DrawImage.Allocation);

                    vkDestroyImageView(m_ContextDevice->Device, m_ContextDevice->Image.DepthImage.ImageView, nullptr);
                    vmaDestroyImage(m_ContextDevice->Allocator, m_ContextDevice->Image.DepthImage.Image, m_ContextDevice->Image.DepthImage.Allocation);
            });

    }

    void MEngine::ResizeSwapchain()
    {
        vkDeviceWaitIdle(m_ContextDevice->Device);

        m_Swapchain.Destroy(m_ContextDevice->Device);

        const VkExtent2D extent = m_Window->Resize();

        m_Swapchain.Create(*m_ContextDevice, extent.width, extent.height);

        m_IsResizeRequested = false;
    }

    VkFormat MEngine::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_ContextDevice->ChosenGPU, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat MEngine::FindDepthFormat() const
    {
        return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void MEngine::CreateRenderPass()
    {
        {
            std::array<VkAttachmentDescription, 2> attachments = {};
            // Color attachment
            attachments[0].format         = m_Swapchain.GetImageFormat();
            attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[0].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            // Depth attachment
            attachments[1].format         = FindDepthFormat();
            attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
            attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorReference = {};
            colorReference.attachment            = 0;
            colorReference.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthReference = {};
            depthReference.attachment            = 1;
            depthReference.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpassDescription    = {};
            subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassDescription.colorAttachmentCount    = 1;
            subpassDescription.pColorAttachments       = &colorReference;
            subpassDescription.pDepthStencilAttachment = &depthReference;
            subpassDescription.inputAttachmentCount    = 0;
            subpassDescription.pInputAttachments       = nullptr;
            subpassDescription.preserveAttachmentCount = 0;
            subpassDescription.pPreserveAttachments    = nullptr;
            subpassDescription.pResolveAttachments     = nullptr;

            // Subpass dependencies for layout transitions
            std::array<VkSubpassDependency, 2> dependencies;

            dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
            dependencies[0].dstSubpass      = 0;
            dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            dependencies[1].srcSubpass      = 0;
            dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
            dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
            dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments           = attachments.data();
            renderPassInfo.subpassCount           = 1;
            renderPassInfo.pSubpasses             = &subpassDescription;
            renderPassInfo.dependencyCount        = static_cast<uint32_t>(dependencies.size());
            renderPassInfo.pDependencies          = dependencies.data();

            if (vkCreateRenderPass(m_ContextDevice->Device, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create render pass!");
            }

            m_MainDeletionQueue.PushFunction([=]() { 
                vkDestroyRenderPass(m_ContextDevice->Device, m_RenderPass, nullptr); 
                });
        }

        m_ImGuiRenderer.CreateViewportRenderPass(m_ContextDevice->Device, FindDepthFormat(), m_Swapchain.GetImageFormat());

    }

    void MEngine::InitCommands()
    {
        const VkCommandPoolCreateInfo commandPoolInfo =
                vkinit::command_pool_create_info(m_ContextDevice->GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(m_ContextDevice->Device, &commandPoolInfo, nullptr, &m_ContextDevice->GetFrameAt(i).CommandPool));

            const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_ContextDevice->GetFrameAt(i).CommandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(m_ContextDevice->Device, &cmdAllocInfo, &m_ContextDevice->GetFrameAt(i).MainCommandBuffer));
        
            m_MainDeletionQueue.PushFunction([=]() { 
                vkDestroyCommandPool(m_ContextDevice->Device, m_ContextDevice->GetFrameAt(i).CommandPool, nullptr);
                });
        }

        VK_CHECK(vkCreateCommandPool(m_ContextDevice->Device, &commandPoolInfo, nullptr, &m_ContextDevice->ImmCommandPool));

        const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_ContextDevice->ImmCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_ContextDevice->Device, &cmdAllocInfo, &m_ContextDevice->ImmCommandBuffer));

        m_MainDeletionQueue.PushFunction([=]() { 
                vkDestroyCommandPool(m_ContextDevice->Device, m_ContextDevice->ImmCommandPool, nullptr);
        });


        // Viewport Command Pool
        m_ImGuiRenderer.CreateViewportCommandPool(m_ContextDevice->Device, m_ContextDevice->GraphicsQueueFamily);
        m_MainDeletionQueue.PushFunction([=]() { 
            vkDestroyCommandPool(m_ContextDevice->Device, m_ImGuiRenderer.GetViewportCommandPool(), nullptr); 
            });

    }
    
    void MEngine::InitSyncStructeres()
    {
        VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

        VK_CHECK(vkCreateFence(m_ContextDevice->Device, &fenceCreateInfo, nullptr, &m_ContextDevice->ImmFence));
        m_MainDeletionQueue.PushFunction([=]() { 
            vkDestroyFence(m_ContextDevice->Device, m_ContextDevice->ImmFence, nullptr); 
            });

        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(m_ContextDevice->Device, &fenceCreateInfo, nullptr, &m_ContextDevice->GetFrameAt(i).RenderFence));


            VK_CHECK(vkCreateSemaphore(m_ContextDevice->Device, &semaphoreCreateInfo, nullptr, &m_ContextDevice->GetFrameAt(i).SwapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(m_ContextDevice->Device, &semaphoreCreateInfo, nullptr, &m_ContextDevice->GetFrameAt(i).RenderSemaphore));
            
            m_MainDeletionQueue.PushFunction([=]() { 
                    vkDestroyFence(m_ContextDevice->Device, m_ContextDevice->GetFrameAt(i).RenderFence, nullptr);
                    vkDestroySemaphore(m_ContextDevice->Device, m_ContextDevice->GetFrameAt(i).SwapchainSemaphore, nullptr);
                    vkDestroySemaphore(m_ContextDevice->Device, m_ContextDevice->GetFrameAt(i).RenderSemaphore, nullptr);
            });
        }

    }

    void MEngine::InitPipelines()
    {
        m_SkyPipeline = std::make_unique<SkyPipeline>();
        m_SkyPipeline->Init(m_ContextDevice->Device, &m_DrawImageDescriptorLayout, m_MainDeletionQueue);
        
        m_RenderPipeline = std::make_shared<RenderPipeline>();
        m_RenderPipeline->Init(m_ContextDevice->Device,
                               m_GPUSceneDataDescriptorLayout,
                               {m_ContextDevice->Image.DrawImage.ImageFormat, m_ContextDevice->Image.DepthImage.ImageFormat}, m_RenderPass);
        
        m_ContextDevice->RenderPipeline = m_RenderPipeline;

    }
    
    void MEngine::InitImgui()
    {   
        VkDescriptorPool outImguiPool;
        m_ImGuiRenderer.Init(*m_ContextDevice, m_Window->GetWindow(), m_Swapchain.GetImageFormat(), m_MainDeletionQueue, outImguiPool);

        m_MainDeletionQueue.PushFunction(
                [=]()
                {
                    ImGui_ImplVulkan_Shutdown();
                    vkDestroyDescriptorPool(m_ContextDevice->Device, outImguiPool, nullptr);
                });

    }

    void MEngine::InitFrameBuffers()
    {
        m_Swapchain.CreateFrameBuffers(m_ContextDevice->Device, m_RenderPass, m_ContextDevice->Image.DepthImage.ImageView);

    }

    void MEngine::PushGuiLayer(ImGuiLayer *inLayer)
    {
        m_GuiLayer = std::unique_ptr<ImGuiLayer>(inLayer);
        m_GuiLayer->Init();
        
    }
    
    void MEngine::InitDescriptors()
    {
        std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3    },
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
        };

        m_GlobalDescriptorAllocator.init_pool(m_ContextDevice->Device, 10, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            m_DrawImageDescriptorLayout = builder.Build(m_ContextDevice->Device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            m_GPUSceneDataDescriptorLayout = builder.Build(m_ContextDevice->Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        m_MainDeletionQueue.PushFunction(
                [&]()
                {
                    vkDestroyDescriptorSetLayout(m_ContextDevice->Device, m_DrawImageDescriptorLayout, nullptr);
                    vkDestroyDescriptorSetLayout(m_ContextDevice->Device, m_GPUSceneDataDescriptorLayout, nullptr);
                });

        m_DrawImageDescriptors = m_GlobalDescriptorAllocator.Allocate(m_ContextDevice->Device, m_DrawImageDescriptorLayout);

        {
            DescriptorWriter writer;
            writer.WriteImage(0, m_ContextDevice->Image.DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            writer.UpdateSet(m_ContextDevice->Device, m_DrawImageDescriptors);
        }
        
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };
        
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            m_ContextDevice->GetFrameAt(i).FrameDescriptors = DescriptorAllocatorGrowable{};
            m_ContextDevice->GetFrameAt(i).FrameDescriptors.Init(m_ContextDevice->Device, 1000, frame_sizes);

            m_MainDeletionQueue.PushFunction([&, i]() { 
                m_ContextDevice->GetFrameAt(i).FrameDescriptors.DestroyPools(m_ContextDevice->Device); 
            });
        }
    }

    uint32_t MEngine::FindMemoryType(const uint32_t inFilterType, const VkMemoryPropertyFlags inProperties) const
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_ContextDevice->ChosenGPU, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if ((inFilterType & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & inProperties) == inProperties)
            {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkCommandBuffer MEngine::BeginSingleTimeCommands(const VkCommandPool &inCommandPool) const
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool        = inCommandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_ContextDevice->Device, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }
    void MEngine::EndSingleTimeCommands(VkCommandBuffer commandBuffer, const VkCommandPool &cmdPool) const
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBuffer;

        vkQueueSubmit(m_ContextDevice->GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_ContextDevice->GraphicsQueue);

        vkFreeCommandBuffers(m_ContextDevice->Device, cmdPool, 1, &commandBuffer);
    }

    void MEngine::InsertImageMemoryBarrier(VkCommandBuffer         cmdbuffer,
                                  VkImage                 image,
                                  VkAccessFlags           srcAccessMask,
                                  VkAccessFlags           dstAccessMask,
                                  VkImageLayout           oldImageLayout,
                                  VkImageLayout           newImageLayout,
                                  VkPipelineStageFlags    srcStageMask,
                                  VkPipelineStageFlags    dstStageMask,
                                  VkImageSubresourceRange subresourceRange) const
    {
        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.srcAccessMask       = srcAccessMask;
        imageMemoryBarrier.dstAccessMask       = dstAccessMask;
        imageMemoryBarrier.oldLayout           = oldImageLayout;
        imageMemoryBarrier.newLayout           = newImageLayout;
        imageMemoryBarrier.image               = image;
        imageMemoryBarrier.subresourceRange    = subresourceRange;

        vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
    }
    
}