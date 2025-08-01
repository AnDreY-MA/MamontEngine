#pragma once 

#include <SDL3/SDL.h>


namespace MamontEngine
{
    struct VkContextDevice;
    struct DeletionQueue;

    class ImGuiRenderer final
    {
    public:
        void Init(VkContextDevice &inContextDevice,
                  SDL_Window      *inWindow,
                  VkFormat         inColorFormat,
                  DeletionQueue   &inDeletionQueue, VkDescriptorPool &outPoolResult);

        void Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, VkExtent2D inRenderExtent);

        void CreateImGuiFrameBuffers(VkDevice inDevie, const VkExtent2D &inExtent, const std::vector<VkImageView>& inSwapchainImageViews);

        void CreateImGuiRenderPass(VkDevice inDevie, VkFormat inSwapChainImageFormat);

        void CreateViewportRenderPass(VkDevice inDevie, const VkFormat inDepthFormat, const VkFormat inSwapImageFormat);

        void CreateCommandBuffers(VkDevice inDevice, const size_t inSizeSwapchainImgageViews);

        void CreateViewportCommandPool(VkDevice inDevie, const uint32_t inGraphicsQueueFamily);
        VkCommandPool& GetViewportCommandPool()
        {
            return m_ViewportCommandPool;
        }

        void CreateViewportImage(VkDevice inDevice, const size_t inSizeSwapchainImages, const VkExtent2D &inSwapchainExtent);
        void CreateViewportImageViews(VkDevice inDevice);

    private:
        VkRenderPass               m_ImGuiRenderPass;
        VkRenderPass m_ViewportRenderPass;
        std::vector<VkFramebuffer> m_ImGuiFramebuffers;
        std::vector<VkCommandBuffer> m_ImGuiCommandBuffers;
        VkCommandPool                m_ImGuiCommandPool;

        //Viewport
        std::vector<VkImage>        m_ViewportImages;
        std::vector<VkImageView>    m_ViewportImageViews;

        std::vector<VkDeviceMemory> m_DstImageMemory;
        VkCommandPool                m_ViewportCommandPool;
    };
}