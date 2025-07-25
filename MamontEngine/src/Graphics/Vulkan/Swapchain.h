#pragma once 

namespace MamontEngine
{
    struct VkContextDevice;
    struct Image;

	class MSwapchain
	{
    public:
        void Init(VkContextDevice &inDevice, const VkExtent2D& inExtent, Image& inImage);

        void Create(const VkContextDevice &inDevice, const uint32_t inWidth, const uint32_t inHeight);

        std::pair<VkResult, uint32_t> AcquireImage(VkDevice inDevice, VkSemaphore &inSemaphore) const;

        VkResult Present(VkQueue inQueue, const VkSemaphore *inRenderSemaphore, const uint32_t inImageIndex);

        void Destroy(VkDevice inDevice);

        const std::vector<VkImage>& GetImages()
        {
            return m_SwapchainImages;
        }

        VkImageView GetImageView(const size_t inIndex)
        {
            return m_SwapchainImageViews.at(inIndex);
        }

        VkExtent2D GetExtent() const
        {
            return m_SwapchainExtent;
        }

        VkFormat GetImageFormat()
        {
            return m_SwapchainImageFormat;
        }

	private:
        VkSwapchainKHR           m_Swapchain;
        VkFormat                 m_SwapchainImageFormat;
        std::vector<VkImage>     m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        VkExtent2D               m_SwapchainExtent;

	};
}