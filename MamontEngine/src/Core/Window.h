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

		void CreateSurface(VkInstance instance, VkSurfaceKHR *surface);

		SDL_Window *GetWindow()
        {
            return m_Window;
		}

		const VkExtent2D& GetExtent() const
		{
            return m_WindowExtent;
		}

		float GetAscpectRatio() const
		{
            return static_cast<float>(m_WindowExtent.width) / static_cast<float>(m_WindowExtent.height);
		}

		void UpdateExtent(const int width, const int height)
		{
            m_WindowExtent.width = width;
            m_WindowExtent.height = height;
		}

		void PollEvents();

		VkExtent2D Resize();

		void InitVulkanWindow(VkInstance inInstance, VkPhysicalDevice inPhysicalDevice, VkDevice inDevice, uint32_t inQueueFamily);
	private:
		void Init();

        void Close();
	private:

        SDL_Window* m_Window;
        ImGui_ImplVulkanH_Window m_VulkanWindow;

        VkExtent2D  m_WindowExtent{1920, 1080};
	};
}