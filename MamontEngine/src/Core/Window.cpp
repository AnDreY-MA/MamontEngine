#include "Core/Window.h"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl3.h"

namespace MamontEngine
{
    void WindowCore::Init()
    {
        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        m_Window = SDL_CreateWindow("Mamont Engine", m_WindowExtent.width, m_WindowExtent.height, window_flags);

        const auto pos = SDL_WINDOWPOS_CENTERED;
        SDL_SetWindowPosition(m_Window, pos, pos);
        SDL_SetWindowSurfaceVSync(m_Window, false);
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