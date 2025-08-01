#pragma once

#include "Graphics/Vulkan/GPUBuffer.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/VkMaterial.h"
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "Core/RenderData.h"

#include "Graphics/Mesh.h"
#include "FrameData.h"


namespace MamontEngine
{
    class WindowCore;
    struct AllocatedImage;
    
    constexpr unsigned int FRAME_OVERLAP = 3;

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

        FrameData &GetCurrentFrame();
        inline FrameData& GetFrameAt(const size_t inIndex)
        {
            return m_Frames.at(inIndex);
        }

        inline void IncrementFrameNumber()
        {
            ++m_FrameNumber;
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

        Image Image;

        VkFence         ImmFence;
        VkCommandBuffer ImmCommandBuffer;
        VkCommandPool   ImmCommandPool;

        VkSampler DefaultSamplerLinear;
        VkSampler DefaultSamplerNearest;

        std::shared_ptr<RenderPipeline> RenderPipeline;

        VkDescriptorSet ViewportDescriptor;

    private:
        std::array<FrameData, FRAME_OVERLAP> m_Frames{};
        int                                  m_FrameNumber{0};


	};
}