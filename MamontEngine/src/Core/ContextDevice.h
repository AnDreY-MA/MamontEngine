#pragma once

#include "vk_mem_alloc.h"
#include "Graphics/Vulkan/GPUBuffer.h"
#include "Graphics/Vulkan/Image.h"


namespace MamontEngine
{
    class WindowCore;
    struct AllocatedImage;

	struct VkContextDevice
	{
        ~VkContextDevice();

        void Init(WindowCore* inWindow);

        AllocatedBuffer CreateBuffer(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage) const;
        void            DestroyBuffer(const AllocatedBuffer &inBuffer) const;

        AllocatedImage  CreateImage(const VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const;
        AllocatedImage  CreateImage(void *inData, VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped);

        void            ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;

        VkInstance               Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkPhysicalDevice         ChosenGPU;
        VkDevice                 Device;
        VkSurfaceKHR             Surface;

        VmaAllocator Allocator;

        VkQueue  GraphicsQueue;
        uint32_t GraphicsQueueFamily;

        AllocatedImage WhiteImage;
        AllocatedImage ErrorCheckerboardImage;

        VkFence         ImmFence;
        VkCommandBuffer ImmCommandBuffer;
        VkCommandPool   ImmCommandPool;
	};
}