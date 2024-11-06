#include "Core/Window.h"

#include <SDL3/SDL.h>

static SDL_Window *mwindow = nullptr;

namespace MamontEngine
{
    void MWindow::Init(std::string_view inName, const VkExtent2D& inExtent)
    {
        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        mwindow = SDL_CreateWindow("Mamont Engine",
                                  inExtent.width,
                                  inExtent.height,
                                  window_flags);

        WindowHandle = mwindow;
    }

    void MWindow::Update()
    {

    }

    void MWindow::Shutdown()
    {
        SDL_DestroyWindow(mwindow);
        SDL_Quit();
    }
}
