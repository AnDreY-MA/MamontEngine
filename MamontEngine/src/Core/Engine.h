#pragma once


namespace MamontEngine
{
    struct FrameData
    {
        VkSemaphore     SwapchainSemaphore;
        VkSemaphore     RenderSemaphore;
        VkFence         RenderFence;
        VkCommandPool   CommandPool;
        VkCommandBuffer MainCommandBuffer;
    };
    constexpr unsigned int FRAME_OVERLAP = 2;

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
        void InitCommants();
        void InitSyncStructeres();

        void CreateSwapchain(const uint32_t inWidth, const uint32_t inHeight);
        void DestroySwapchain();

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

	};
}