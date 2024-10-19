#pragma once

#include "vk_mem_alloc.h"
#include "VkDestriptor.h"
#include "pch.h"

struct MPipeline
{
    VkPipeline Pipeline;
};

namespace MamontEngine
{
    
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct DeletionQueue
    {
    public:
        void PushFunction(std::function < void()>&& inFunction)
        {
            deletors.push_back(inFunction);
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }

            deletors.clear();
        }

    private:
        std::deque<std::function<void()>> deletors;

    };

    struct FrameData
    {
        VkSemaphore     SwapchainSemaphore;
        VkSemaphore     RenderSemaphore;
        VkFence         RenderFence;
        VkCommandPool   CommandPool;
        VkCommandBuffer MainCommandBuffer;

        DeletionQueue Deleteions;

    };

    struct ComputePushConstants
    {
        glm::vec4 Data1;
        glm::vec4 Data2;
        glm::vec4 Data3;
        glm::vec4 Data4;
    };

    struct ComputeEffect
    {
        const char *Name;
        ::VkPipeline  Pipeline;
        VkPipelineLayout Layout;
        
        //ComputePushConstants Data;
    };


	class MEngine
	{
    public:
        void Init();
        void Run();
        void Cleanup();

        void Draw();

        static MEngine& Get();

    private:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructeres();

        void CreateSwapchain(const uint32_t inWidth, const uint32_t inHeight);
        void DestroySwapchain();

        void DrawBackground(VkCommandBuffer inCmd);
        
        void InitDescriptors();

        void InitPipelines();
        void InitBackgroundPipelines();

        void InitImgui();
        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction);
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView);

    private:
        bool       m_IsInitialized{false};
        int        m_FrameNumber{0};
        bool       m_StopRendering{false};
        VkExtent2D m_WindowExtent{1700, 900};

        void *m_Window{nullptr};

        VkInstance               m_Instance;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkPhysicalDevice         m_ChosenGPU;
        VkDevice                 m_Device;
        VkSurfaceKHR             m_Surface;

        VkSwapchainKHR           m_Swapchain;
        VkFormat                 m_SwapchainImageFormat;
        std::vector<VkImage>     m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        VkExtent2D               m_SwapchainExtent;

        FrameData m_Frames[FRAME_OVERLAP];
        /*FrameData &GetCurrentFrame()
        {
            return m_Frames[m_FrameNumber & FRAME_OVERLAP];
        }*/

        VkQueue m_GraphicsQueue;
        uint32_t m_GraphicsQueueFamily;

        DeletionQueue m_MainDeletionQueue;

        VmaAllocator m_Allocator;

        AllocatedImage m_DrawImage;
        VkExtent2D     m_DrawExtent;

        VkDescriptor::DescriptorAllocator m_GlobalDescriptorAllocator;
        VkDescriptorSet                   m_DrawImageDescriptors;
        VkDescriptorSetLayout             m_DrawImageDescriptorLayout;

        VkPipelineLayout m_GradientPipelineLayout;
        //MPipeline        m_Pipeline;

        VkFence         m_ImmFence;
        VkCommandBuffer m_ImmCommandBuffer;
        VkCommandPool   m_ImmCommandPool;


	};
}