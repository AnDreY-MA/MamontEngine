#include "Core/Window.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include "imgui/backends/imgui_impl_sdl3.h"
#include "Core/Engine.h"
#include "Core/ContextDevice.h"

namespace MamontEngine
{
    static void SetupVulkanWindow(ImGui_ImplVulkanH_Window *wd, VkContextDevice& inContextDevice, VkSurfaceKHR surface, int width, int height)
    {
        wd->Surface = surface;

        // Check for WSI support
        VkBool32 res;
        vkGetPhysicalDeviceSurfaceSupportKHR(inContextDevice.ChosenGPU, inContextDevice.GraphicsQueueFamily, wd->Surface, &res);
        if (res != VK_TRUE)
        {
            fprintf(stderr, "Error no WSI support on physical device 0\n");
            exit(-1);
        }

        // Select Surface Format
        const VkFormat requestSurfaceImageFormat[]     = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        wd->SurfaceFormat                              = ImGui_ImplVulkanH_SelectSurfaceFormat(
                inContextDevice.ChosenGPU, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

        // Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
#else
        VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
#endif
        wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(inContextDevice.ChosenGPU, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
        // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

        // Create SwapChain, RenderPass, Framebuffer, etc.
        //IM_ASSERT(g_MinImageCount >= 2);
        ImGui_ImplVulkanH_CreateOrResizeWindow(inContextDevice.Instance,
                                               inContextDevice.ChosenGPU,
                                               inContextDevice.Device,
                                               wd,
                                               inContextDevice.GraphicsQueueFamily,
                                               nullptr,
                                               width,
                                               height,
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

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        m_Window = SDL_CreateWindow("Mamont Engine", m_WindowExtent.width, m_WindowExtent.height, window_flags);

        const auto pos = SDL_WINDOWPOS_CENTERED;

        SDL_SetWindowPosition(m_Window, pos, pos);
        SDL_SetWindowSurfaceVSync(m_Window, false);
        //SetupVulkanWindow(wd, w, h);

        fmt::println("Win sync {}", SDL_SyncWindow(m_Window));
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