#include "Core/Window.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include "imgui/backends/imgui_impl_sdl3.h"
#include "Core/Engine.h"

namespace MamontEngine
{
    void WindowCore::SetupVulkanWindow(VkContextDevice &inContextDevice, VkSurfaceKHR surface, int width, int height)
    {
        m_VulkanWindow          = new ImGui_ImplVulkanH_Window();
        m_VulkanWindow->Surface = surface;
        int w                   = 0;
        int h                   = 0;
        SDL_GetWindowSize(m_Window, &w, &h);

        // Check for WSI support
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(inContextDevice.ChosenGPU, inContextDevice.GraphicsQueueFamily, m_VulkanWindow->Surface, &res);
        if (res != VK_TRUE)
        {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }

        // Select Surface Format
        constexpr VkFormat requestSurfaceImageFormat[] = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
        constexpr VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        m_VulkanWindow->SurfaceFormat                      = ImGui_ImplVulkanH_SelectSurfaceFormat(inContextDevice.ChosenGPU,
                                                                              m_VulkanWindow->Surface,
                                                                              requestSurfaceImageFormat,
                                                                              IM_ARRAYSIZE(requestSurfaceImageFormat),
                                                                              requestSurfaceColorSpace);

        // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
        constexpr VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
        constexpr VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
        m_VulkanWindow->PresentMode =
                ImGui_ImplVulkanH_SelectPresentMode(inContextDevice.ChosenGPU, m_VulkanWindow->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));

        // Create SwapChain, RenderPass, Framebuffer, etc.
        //IM_ASSERT(g_MinImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(inContextDevice.Instance,
                                               inContextDevice.ChosenGPU,
                                               inContextDevice.Device,
                                               m_VulkanWindow,
                                               inContextDevice.GraphicsQueueFamily,
                                               nullptr,
                                               w,
                                               h,
                                               1);

    }

    WindowCore::WindowCore()
    {
        Init();
    }
    WindowCore::~WindowCore()
    {
        Close();
    }
    
    void WindowCore::Init()
    {
        SDL_Init(SDL_INIT_VIDEO);

        constexpr SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        m_Window = SDL_CreateWindow("Mamont Engine", m_WindowExtent.width, m_WindowExtent.height, window_flags);

        const auto pos = SDL_WINDOWPOS_CENTERED;

        SDL_SetWindowPosition(m_Window, pos, pos);
        SDL_SetWindowSurfaceVSync(m_Window, false);
        //SetupVulkanWindow(wd, w, h);
    }

    void WindowCore::Close()
    {
        SDL_DestroyWindow(m_Window);
    }

    void WindowCore::CreateSurface(VkInstance instance, VkSurfaceKHR *surface)
    {
        if (!SDL_Vulkan_CreateSurface(m_Window, instance, nullptr, surface))
        {
            fmt::print("Surface is false");
            exit(1);
        }
    }

    VkExtent2D WindowCore::Resize()
    {
        int w = 0, h = 0;
        SDL_GetWindowSize(m_Window, &w, &h);
        UpdateExtent(w, h);
        return m_WindowExtent;
    }

}