#pragma once

#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "Graphics/RenderData.h"

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

        //void PrepareFrame(); 

        void DestroyFrameData();

        AllocatedImage
        CreateImage(const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false, uint32_t arrayLayers = 1) const;
        AllocatedImage CreateImage(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const;

        void InitDefaultImages();

        void DestroyImage(const AllocatedImage &inImage) const;

        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;

        void InitCommands();

        void InitSyncStructeres();

        void InitDescriptors();

        void InitSamples();

        void InitSceneBuffers();

        void InitShadowImages();

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
        VkSurfaceKHR             Surface{VK_NULL_HANDLE};

        VkFence TransferFence{VK_NULL_HANDLE};

        std::vector<VkSemaphore> RenderCopleteSemaphores{};

        AllocatedImage WhiteImage;
        AllocatedImage ErrorCheckerboardImage;

        Image Image;

        std::vector<AllocatedImage> PickingImages;

        struct DepthImage
        {
            AllocatedImage Image;
            VkSampler      Sampler;
        } CascadeDepthImage;

        std::array<Cascade, CASCADECOUNT> Cascades{};

        MSwapchain Swapchain;

        VkSampler DefaultSamplerLinear{VK_NULL_HANDLE};
        VkSampler DefaultSamplerNearest{VK_NULL_HANDLE};

        DescriptorAllocatorGrowable GlobalDescriptorAllocator;
        VkDescriptorSet       DrawImageDescriptors{VK_NULL_HANDLE};
        VkDescriptorSetLayout DrawImageDescriptorLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout GPUSceneDataDescriptorLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout MaterialDescriptorLayout{VK_NULL_HANDLE};
        
        RenderPipeline* RenderPipeline;

    private:
        void DestroyImages() const;
       
        void DestroyCommands();
       
        void DestroySyncStructeres();

        void DestroyDescriptors();
        
        void DestroySceneBuffers();

        VkFormat GetSupportedDepthFormat(bool checkSamplingSupport);

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