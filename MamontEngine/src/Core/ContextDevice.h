#pragma once

#include "Graphics/Resources/Texture.h"

#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"

#include "Core/FrameData.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Graphics/Resources/Models/Mesh.h"
#include "Graphics/Vulkan/Swapchain.h"
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

        bool BeginFrame();
        
        bool EndFrame(VkCommandBuffer cmd);

        void DestroyFrameData();

        void InitShadowImages();

        void ResizeSwapchain(const VkExtent2D &inWindowExtent);

        FrameData       &GetCurrentFrame();
        const FrameData &GetCurrentFrame() const;

        inline FrameData &GetFrameAt(const size_t inIndex)
        {
            return m_Frames.at(inIndex);
        }

        inline void IncrementFrameNumber()
        {
            m_FrameNumber = (m_FrameNumber + 1) % FRAME_OVERLAP;
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

        VkInstance GetInstance() const
        {
            return Instance;
        }

        inline bool IsResizeRequest() const
        {
            return m_IsResizeRequested;
        }

        std::vector<VkSemaphore> RenderCopleteSemaphores{};

        Texture DrawImage;
        Texture DepthImage;

        std::vector<Texture> PickingImages;

        Texture CascadeDepthImage;

        MSwapchain Swapchain;

        DescriptorAllocatorGrowable GlobalDescriptorAllocator;
        VkDescriptorSet             DrawImageDescriptors{VK_NULL_HANDLE};

        VkDescriptorSetLayout DrawImageDescriptorLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout RenderDescriptorLayout{VK_NULL_HANDLE};
        VkDescriptorSetLayout GPUSceneDataDescriptorLayout{VK_NULL_HANDLE};

        std::shared_ptr<RenderPipeline> RenderPipeline;
        void            InitDescriptors();

        void CreatePrefilteredCubeTexture(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &&inDrawSkyboxFunc);

    private:
#pragma region Initialize functions
        void InitSwapchain(const VkExtent2D &inWindowExtent);

        void InitImage();

        void InitCommands();

        void InitSyncStructeres();


        void InitSceneBuffers();

        void InitTracyContext();
#pragma endregion

#pragma region Destroy functions
        void DestroyImages();

        void DestroyCommands();

        void DestroySyncStructeres();

        void DestroyDescriptors();

        void DestroySceneBuffers();
#pragma endregion

        VkFormat GetSupportedDepthFormat(bool checkSamplingSupport);

    private:
        std::array<FrameData, FRAME_OVERLAP> m_Frames{};
        size_t                               m_FrameNumber{0};

        VkInstance               Instance{VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT DebugMessenger{VK_NULL_HANDLE};
        VkSurfaceKHR             Surface{VK_NULL_HANDLE};

        VkQueue  m_GraphicsQueue{VK_NULL_HANDLE};
        uint32_t m_GraphicsQueueFamily{0};
        std::unique_ptr<Texture> m_SkyboxTexture;
        std::unique_ptr<Texture> m_BRDFUTTexture;
        std::unique_ptr<Texture> m_PrefilteredCubeTexture;
        std::unique_ptr<Texture> m_IrradianceTexture;

        bool m_IsResizeRequested{false};

        struct TracyInfo
        {
            VkCommandPool                                      CommandPool{VK_NULL_HANDLE};
            VkCommandBuffer                                    CommandBuffer{VK_NULL_HANDLE};
            PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT GetPhysicalDeviceCalibrateableTimeDomainsEXT = nullptr;
            PFN_vkGetCalibratedTimestampsEXT                   GetCalibratedTimestampsEXT                   = nullptr;
        } m_TracyInfo;
    };
} // namespace MamontEngine
