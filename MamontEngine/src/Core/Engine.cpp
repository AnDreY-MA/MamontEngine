#include "Engine.h"

#include "VkInitializers.h"
#include "VkImages.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_events.h>

#include <chrono>
#include <thread>
#include <VkBootstrap.h>

static SDL_Window* window = nullptr;

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;
    MEngine       *loadedEngine         = nullptr;
    MEngine       &MEngine::Get()
    {
        return *loadedEngine;
    }

    void MEngine::Init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

        window = SDL_CreateWindow(
            "Mamont Engine", 
            //SDL_WINDOWPOS_UNDEFINED, 
            //SDL_WINDOWPOS_UNDEFINED, 
            m_WindowExtent.width,
            m_WindowExtent.height,
            window_flags);

        m_Window = window;

        InitVulkan();
        InitSwapchain();
        InitCommants();
        InitSyncStructeres();

        m_IsInitialized = true;
    }

    void MEngine::Run()
    {
        SDL_Event event;
        bool      bQuit{false};

        while (!bQuit)
        {
            while (SDL_PollEvent(&event) != 0)
            { 
                // close the window when user alt-f4s or clicks the X button
                if (event.type == SDL_EVENT_QUIT)
                    bQuit = true;

                /*if (event.type == SDL_WINDOWEVENT)
                {
                    
                }*/
                if (event.window.type == SDL_EVENT_WINDOW_MINIMIZED)
                {
                    m_StopRendering = true;
                }
                if (event.window.type == SDL_EVENT_WINDOW_RESTORED)
                {
                    m_StopRendering = false;
                }
            }

            if (m_StopRendering)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else
            {
                Draw();
            }

        }
    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            vkDeviceWaitIdle(m_Device);
            for (int i = 0; i < FRAME_OVERLAP; i++)
            {
                vkDestroyCommandPool(m_Device, m_Frames[i].CommandPool, nullptr);

                vkDestroyFence(m_Device, m_Frames[i].RenderFence, nullptr);
                vkDestroySemaphore(m_Device, m_Frames[i].RenderSemaphore, nullptr);
                vkDestroySemaphore(m_Device, m_Frames[i].SwapchainSemaphore, nullptr);
            }
            DestroySwapchain();

            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            vkDestroyDevice(m_Device, nullptr);
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
            vkDestroyInstance(m_Instance, nullptr);
            SDL_DestroyWindow(window);
            m_Window = nullptr;
        }

        loadedEngine = nullptr;
    }

    void MEngine::Draw()
    {
        
        //if (m_Frames[m_FrameNumber].RenderFence == VK_NULL_HANDLE)
        //{
        //    fmt::println("RenderFence is not created correctly!");
        //    return;
        //}

        //// Проверка состояния флага
        //VkResult status = vkGetFenceStatus(m_Device, m_Frames[m_FrameNumber].RenderFence);
        //if (status == VK_ERROR_DEVICE_LOST)
        //{
        //    fmt::println("Device lost. Unable to wait for fence.");
        //    return;
        //}
        fmt::println("Waiting for fence...");
        VK_CHECK(vkWaitForFences(m_Device, 1, &m_Frames[m_FrameNumber].RenderFence, true, 1000000000));
        fmt::println("Fence signaled.");
        VK_CHECK(vkResetFences(m_Device, 1, &m_Frames[m_FrameNumber].RenderFence));

        uint32_t swapchainImageIndex;
        VkResult e = vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, m_Frames[m_FrameNumber].SwapchainSemaphore, nullptr, &swapchainImageIndex);
        /*if (e == VK_ERROR_OUT_OF_DATE_KHR)
        {
            return;
        }*/

        VkCommandBuffer cmd = m_Frames[m_FrameNumber].MainCommandBuffer;
        VK_CHECK(vkResetCommandBuffer(cmd, 0));
        
        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        VkClearColorValue clearValue;
        float       flash = std::abs(std::sin(m_FrameNumber / 12.f));
        fmt::println("Flash={}", flash);
        clearValue                         = {{0.0f, 0.0f, flash, 1.0f}};
        VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

        vkCmdClearColorImage(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

        VkUtil::transition_image(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        VkSemaphoreSubmitInfo     waitInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, m_Frames[m_FrameNumber].SwapchainSemaphore);
        VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_Frames[m_FrameNumber].RenderSemaphore);

        VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_Frames[m_FrameNumber].RenderFence));

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.pNext            = nullptr;
        presentInfo.pSwapchains      = &m_Swapchain;
        presentInfo.swapchainCount   = 1;

        presentInfo.pWaitSemaphores    = &m_Frames[m_FrameNumber].RenderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VK_CHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

        m_FrameNumber = (m_FrameNumber + 1) % FRAME_OVERLAP;
    }

    void MEngine::InitVulkan()
    {
        vkb::InstanceBuilder builder;

        auto instRet = builder.set_app_name("Mamont App")
            .request_validation_layers(bUseValidationLayers)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 0)
            .build();

        vkb::Instance vkbInstance = instRet.value();
        m_Instance                = vkbInstance.instance;
        m_DebugMessenger          = vkbInstance.debug_messenger;

        if (!m_Instance || !m_DebugMessenger)
        {
            fmt::print("NULL");
        }

        if (!SDL_Vulkan_CreateSurface((SDL_Window*)m_Window, m_Instance, nullptr, &m_Surface))
        {
            fmt::print("Surface is false");
            return;
        }

        VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features.dynamicRendering = true;
        features.synchronization2 = true;
    
        VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features12.bufferDeviceAddress = true;
        features12.descriptorIndexing  = true;


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        vkb::PhysicalDevice         physicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
                                                     .set_surface(m_Surface)
                                                     //.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select()
            .value();

        vkb::DeviceBuilder deviceBuilder{physicalDevice};

        vkb::Device vkbDevice = deviceBuilder.build().value();

        m_Device = vkbDevice.device;
        m_ChosenGPU = physicalDevice.physical_device;
        fmt::print("Name {}", physicalDevice.name);

        m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    }
    void MEngine::InitSwapchain()
    {
        CreateSwapchain(m_WindowExtent.width, m_WindowExtent.height);
    }
    void MEngine::InitCommants()
    {
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool));

            VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_Frames[i].CommandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].MainCommandBuffer));
        }
    }
    void MEngine::InitSyncStructeres()
    {
        VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            //VK_CHECK();
            if (vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence) != VK_SUCCESS)
            {
                fmt::println("Failed Fence");
            }

            VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].RenderSemaphore));

        }
    }

    void MEngine::CreateSwapchain(const uint32_t inWidth, const uint32_t inHeight)
    {
        vkb::SwapchainBuilder swapchainBuilder{m_ChosenGPU, m_Device, m_Surface};
        m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        vkb::Swapchain vkbSwapchain = swapchainBuilder
            .set_desired_format(VkSurfaceFormatKHR{.format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(inWidth, inHeight)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

        m_SwapchainExtent = vkbSwapchain.extent;
        m_Swapchain       = vkbSwapchain.swapchain;
        m_SwapchainImages = vkbSwapchain.get_images().value();
        m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
    }
    void MEngine::DestroySwapchain()
    {
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

        for (int i = 0; i < m_SwapchainImageViews.size(); i++)
        {
            vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
        }
    }
}