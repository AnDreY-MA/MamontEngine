#pragma once 


namespace MamontEngine
{
    struct VkContextDevice;
    struct Image;

	class MSwapchain
	{
    public:
        void Init(const VkSurfaceKHR inSurface, const VkExtent2D &inExtent);

        void Create(const VkSurfaceKHR inSurface, const VkExtent2D &inExtent);

        void ReCreate(const VkSurfaceKHR inSurface, const VkExtent2D &inExtent);

        std::pair<VkResult, uint32_t> AcquireImage(VkDevice inDevice, VkSemaphore inSemaphore);

        VkResult Present(VkQueue inQueue, const VkSemaphore *inRenderSemaphore, const uint32_t inImageIndex) const;

        void Destroy(VkDevice inDevice);

        inline VkImage GetCurrentImage() const
        {
            return m_SwapchainImages[m_CurrentImageIndex];
        }

        inline VkImage GetImageAt(const size_t inIndex) const
        {
            return m_SwapchainImages.at(inIndex);
        }

        const std::vector<VkImage>& GetImages()
        {
            return m_SwapchainImages;
        }

        VkImageView& GetImageView(const size_t inIndex)
        {
            return m_SwapchainImageViews.at(inIndex);
        }
        const VkImageView &GetImageView(const size_t inIndex) const
        {
            return m_SwapchainImageViews.at(inIndex);
        }
        std::vector<VkImageView> GetImageViews() const
        {
            return m_SwapchainImageViews;
        }

        const VkExtent2D& GetExtent() const
        {
            return m_SwapchainExtent;
        }

        VkFormat GetImageFormat() const
        {
            return m_SwapchainImageFormat;
        }

        uint32_t GetCurrentImageIndex() const
        {
            return m_CurrentImageIndex;
        }

    private:
        vkb::Swapchain           m_vkbSwapchain;
        VkSwapchainKHR           m_Swapchain;
        VkFormat                 m_SwapchainImageFormat;
        std::vector<VkImage>     m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        VkExtent2D               m_SwapchainExtent;
        uint32_t                 m_CurrentImageIndex{0};

	};
}