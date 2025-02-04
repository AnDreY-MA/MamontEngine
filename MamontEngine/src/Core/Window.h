#pragma once

#include <SDL3/SDL.h>


namespace MamontEngine
{
	class WindowCore
	{
    public:
        void Init();

        void Close();

		void CreateSurface(VkInstance instance, VkSurfaceKHR *surface);

		SDL_Window *GetWindow()
        {
            return m_Window;
		}

		VkExtent2D GetExtent()
		{
            return m_WindowExtent;
		}

		void UpdateExtent(const int width, const int height)
		{
            m_WindowExtent.width = width;
            m_WindowExtent.height = height;
		}

		VkExtent2D Resize();

	private:

        SDL_Window* m_Window;

        VkExtent2D  m_WindowExtent{1700, 900};
	};
}