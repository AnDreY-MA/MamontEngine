#pragma once

#include "vk_mem_alloc.h"
#include "Graphics/Vulkan/GPUBuffer.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/VkMaterial.h"
#include "Core/RenderData.h"

#include "Graphics/Mesh.h"

namespace MamontEngine
{
    class WindowCore;
    struct AllocatedImage;

	struct VkContextDevice
	{
        VkContextDevice() = default;

        ~VkContextDevice();

        void Init(WindowCore* inWindow);

        AllocatedBuffer CreateBuffer(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage) const;
        void            DestroyBuffer(const AllocatedBuffer &inBuffer) const;

        GPUMeshBuffers CreateGPUMeshBuffer(std::span<uint32_t> inIndices, std::span<Vertex> inVertices);

        AllocatedImage  CreateImage(const VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const;
        AllocatedImage  CreateImage(void *inData, VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped);

        void DestroyImage(const AllocatedImage &inImage);

        void            ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;

        MaterialInstance
        WriteMetalMaterial(EMaterialPass pass, const GLTFMetallic_Roughness::MaterialResources &resources, DescriptorAllocatorGrowable &descriptorAllocator)
        {
            return MetalRoughMaterial.WriteMaterial(Device, pass, resources, descriptorAllocator);
        }

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

        VkSampler DefaultSamplerLinear;
        VkSampler DefaultSamplerNearest;

        GLTFMetallic_Roughness MetalRoughMaterial;

	};
}