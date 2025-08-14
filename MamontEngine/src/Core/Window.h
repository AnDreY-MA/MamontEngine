#pragma once

#include <SDL3/SDL.h>
#include <backends/imgui_impl_vulkan.h>
#include "Core/ContextDevice.h"

namespace MamontEngine
{
	class WindowCore
	{
    public:
        explicit WindowCore();
        ~WindowCore();

        void Init();

        void Close();

		void CreateSurface(VkInstance instance, VkSurfaceKHR *surface);

		void SetupVulkanWindow(VkContextDevice &inContextDevice, VkSurfaceKHR surface, int width, int height);

		SDL_Window *GetWindow()
        {
            return m_Window;
		}

		const VkExtent2D& GetExtent() const
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
        ImGui_ImplVulkanH_Window *m_VulkanWindow;

        VkExtent2D  m_WindowExtent{1900, 1060};
	};
}