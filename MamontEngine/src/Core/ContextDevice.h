#pragma once

#include "Graphics/Vulkan/GPUBuffer.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "Core/RenderData.h"

#include "Graphics/Mesh.h"
#include "FrameData.h"
#include "Graphics/Vulkan/Swapchain.h"
#include "Graphics/Devices/PhysicalDevice.h"


namespace MamontEngine
{
    class WindowCore;
    struct AllocatedImage;
    
    constexpr size_t FRAME_OVERLAP = 3;

	struct VkContextDevice
	{
        VkContextDevice() = default;

        ~VkContextDevice();

        void Init(WindowCore* inWindow);

        AllocatedBuffer CreateBuffer(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage) const;
        void            DestroyBuffer(const AllocatedBuffer &inBuffer) const;

        MeshBuffer CreateGPUMeshBuffer(std::span<uint32_t> inIndices, std::span<Vertex> inVertices);

        AllocatedImage  CreateImage(const VkExtent3D& inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const;
        AllocatedImage CreateImage(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped);

        void InitDefaultImages();

        void DestroyImage(const AllocatedImage &inImage);

        void            ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;

        void InitCommands(DeletionQueue& inDeletionQueue);

        void InitSyncStructeres();
        void DestroySyncStructeres();

        void InitDescriptors();
        void DestroyDescriptors();

        void InitSamples();

        void CreateAllFrameOffscreans(const VkExtent2D &inExtent, const VkFormat inFormat, std::vector<VkImageView> inImageView);
        void CreateFrameOffscreen(FrameData &inFrame, const VkExtent2D &inExtent, const VkFormat inFormat, VkImageView inImageView);

        void InitSwapchain(const VkExtent2D &inWindowExtent);
        void ResizeSwapchain(const VkExtent2D &inWindowExtent);

        void DestroyImage();

        FrameData &GetCurrentFrame();
        inline FrameData& GetFrameAt(const size_t inIndex)
        {
            return m_Frames.at(inIndex);
        }

        inline void IncrementFrameNumber()
        {
            ++m_FrameNumber;
        }

        inline size_t GetFrame() const
        {
            return m_FrameNumber;
        }

        VkPhysicalDevice GetPhysicalDevice() const;

        VkInstance               Instance;
        VkDebugUtilsMessengerEXT DebugMessenger;
        VkDevice                 Device;
        VkSurfaceKHR             Surface;

        VmaAllocator Allocator;

        VkQueue  GraphicsQueue;
        uint32_t GraphicsQueueFamily;

        AllocatedImage WhiteImage;
        AllocatedImage ErrorCheckerboardImage;

        Image Image;

        MSwapchain Swapchain;

        VkFence         ImmFence;
        VkCommandBuffer ImmCommandBuffer;
        VkCommandPool   ImmCommandPool;

        VkSampler DefaultSamplerLinear;
        VkSampler DefaultSamplerNearest;

        DescriptorAllocator   GlobalDescriptorAllocator;
        VkDescriptorSet       DrawImageDescriptors;
        VkDescriptorSetLayout DrawImageDescriptorLayout;
        VkDescriptorSetLayout GPUSceneDataDescriptorLayout;

        VkRenderPass RenderPass;

        RenderPipeline* RenderPipeline;

        VkDescriptorSet ViewportDescriptor;

    private:
        std::array<FrameData, FRAME_OVERLAP> m_Frames{};
        size_t                                  m_FrameNumber{0};

        std::unique_ptr<PhysicalDevice> m_PhysicalDevice;
	};
}