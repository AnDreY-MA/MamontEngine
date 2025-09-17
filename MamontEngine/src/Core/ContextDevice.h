#pragma once

#include "Graphics/Vulkan/MeshBuffer.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "Core/RenderData.h"

#include "Graphics/Mesh.h"
#include "FrameData.h"
#include "Graphics/Vulkan/Swapchain.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Utils/Profile.h"

namespace MamontEngine
{
    class WindowCore;
    struct AllocatedImage;
    
    constexpr size_t FRAME_OVERLAP = 3;

	struct VkContextDevice
	{
        explicit VkContextDevice(WindowCore *inWindow);

        ~VkContextDevice();

        void DestroyFrameData();

        AllocatedBuffer CreateBuffer(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage) const;
        void            DestroyBuffer(const AllocatedBuffer &inBuffer) const;

        MeshBuffer CreateGPUMeshBuffer(std::span<uint32_t> inIndices, std::span<Vertex> inVertices) const;

        AllocatedImage  CreateImage(const VkExtent3D& inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const;
        AllocatedImage CreateImage(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const;

        void InitDefaultImages();

        void DestroyImage(const AllocatedImage &inImage) const;

        void            ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;

        void InitCommands();

        void InitSyncStructeres();

        void InitDescriptors();

        void InitSamples();

        void InitSceneBuffers();

        void InitSwapchain(const VkExtent2D &inWindowExtent);
        void ResizeSwapchain(const VkExtent2D &inWindowExtent);

        void InitTracyContext();

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

        VkQueue GetGraphicsQueue() const
        {
            return m_GraphicsQueue;
        }

        uint32_t GetGraphicsQueueFamily() const
        {
            return m_GraphicsQueueFamily;
        }

        VkPhysicalDevice GetPhysicalDevice() const;

        VkInstance               Instance{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT DebugMessenger{VK_NULL_HANDLE};
        VkDevice                 Device{VK_NULL_HANDLE};
        VkSurfaceKHR             Surface{VK_NULL_HANDLE};

        VmaAllocator Allocator;

        AllocatedImage WhiteImage;
        AllocatedImage ErrorCheckerboardImage;

        Image Image;

        MSwapchain Swapchain;

        VkSampler DefaultSamplerLinear{VK_NULL_HANDLE};
        VkSampler DefaultSamplerNearest{VK_NULL_HANDLE};

        DescriptorAllocator   GlobalDescriptorAllocator;
        VkDescriptorSet       DrawImageDescriptors{VK_NULL_HANDLE};
        VkDescriptorSetLayout DrawImageDescriptorLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout GPUSceneDataDescriptorLayout{VK_NULL_HANDLE};

        VkRenderPass RenderPass;

        RenderPipeline* RenderPipeline;

    private:
        void DestroyImage();
       
        void DestroyCommands();
       
        void DestroySyncStructeres();

        void DestroyDescriptors();
        
        void DestroySceneBuffers();

    private:
        std::array<FrameData, FRAME_OVERLAP> m_Frames{};
        size_t                                  m_FrameNumber{0};

        std::unique_ptr<PhysicalDevice> m_PhysicalDevice;

        VkFence         m_ImmFence{VK_NULL_HANDLE};
        VkCommandBuffer m_ImmCommandBuffer{VK_NULL_HANDLE};
        VkCommandPool   m_ImmCommandPool{VK_NULL_HANDLE};

        VkQueue  m_GraphicsQueue{VK_NULL_HANDLE};
        uint32_t m_GraphicsQueueFamily{0};

        struct TracyInfo
        {
            VkCommandPool                                      CommandPool{VK_NULL_HANDLE};
            VkCommandBuffer                                    CommandBuffer{VK_NULL_HANDLE};
            PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT GetPhysicalDeviceCalibrateableTimeDomainsEXT = nullptr;
            PFN_vkGetCalibratedTimestampsEXT                   GetCalibratedTimestampsEXT = nullptr;
        } m_TracyInfo;

	};
}