#include "Core/Window.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include "imgui/backends/imgui_impl_sdl3.h"
#include "Core/Engine.h"

namespace MamontEngine
{
    static bool SDLCALL CloseWindowEvent(void*, SDL_Event* event)
    {
      /*  Log::Info("Call CloseWindowEvent");

        std::printf("Call CloseWindowEvent: %u\n", event->type);*/

        return true;
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

        constexpr SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED;

        m_Window = SDL_CreateWindow("Mamont Engine", m_WindowExtent.width, m_WindowExtent.height, window_flags);

        const auto pos = SDL_WINDOWPOS_CENTERED;

        SDL_SetWindowPosition(m_Window, pos, pos);
        SDL_SetWindowSurfaceVSync(m_Window, false);

        SDL_AddEventWatch(CloseWindowEvent, nullptr);

    }

    void WindowCore::Close()
    {
        SDL_RemoveEventWatch(CloseWindowEvent, nullptr);

        SDL_DestroyWindow(m_Window);
    }

    void WindowCore::CreateSurface(VkInstance instance, VkSurfaceKHR *surface)
    {
        if (!SDL_Vulkan_CreateSurface(m_Window, instance, nullptr, surface))
        {
            fmt::print("Surface is false");
            exit(1);
        }

        m_VulkanWindow.Surface = *surface;

    }

    void WindowCore::InitVulkanWindow(VkInstance inInstance, VkPhysicalDevice inPhysicalDevice, VkDevice inDevice, uint32_t inQueueFamily)
    {
        const VkFormat requestSurfaceImageFormat[]     = {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM};
        const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        m_VulkanWindow.SurfaceFormat                    = ImGui_ImplVulkanH_SelectSurfaceFormat(
                inPhysicalDevice, m_VulkanWindow.Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

        const std::array < VkPresentModeKHR, 3> present_modes = {VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR};
        m_VulkanWindow.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(inPhysicalDevice, m_VulkanWindow.Surface, present_modes.data(), present_modes.size());


        ImGui_ImplVulkanH_CreateOrResizeWindow(
                inInstance, inPhysicalDevice, inDevice, &m_VulkanWindow, inQueueFamily, nullptr, m_WindowExtent.width, m_WindowExtent.height, 2);

    }

    void WindowCore::PollEvents()
    {
        SDL_Event event;

        while (SDL_PollEvent(&event) != 0)
        {
            ImGui_ImplSDL3_ProcessEvent(&event);

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