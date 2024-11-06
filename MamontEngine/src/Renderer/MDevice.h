#pragma once

#include "vk_mem_alloc.h"

struct SDL_Window;


namespace MamontEngine
{

	class MDevice
	{
    public:

        /*MDevice(const MDevice &) = delete;
        MDevice &operator=(const MDevice &) = delete;*/

        void Init(SDL_Window *inWindow);
        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;

        void Shutdown() const;

        void DestroyImage(const struct AllocatedImage &inImage) const;

        VkDevice GetDevice()
        {
            return m_Device;
        }

        VmaAllocator GetAllocator()
        {
            return m_Allocator;
        }

	private:
        VkInstance               m_Instance;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkPhysicalDevice         m_ChosenGPU;
        VkDevice                 m_Device;
        VkSurfaceKHR             m_Surface;
        VkQueue                  m_GraphicsQueue;
        uint32_t                 m_GraphicsQueueFamily;

        VkCommandBuffer          m_ImmCommandBuffer;
        VkFence         m_ImmFence;


        VmaAllocator m_Allocator;

        friend class MEngine;

	};
}