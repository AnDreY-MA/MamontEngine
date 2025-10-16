#include "Graphics/Vulkan/Swapchain.h"

#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Core/ContextDevice.h"
#include "Core/VkInitializers.h"
#include "Graphics/Devices/LogicalDevice.h"

namespace MamontEngine
{
    void MSwapchain::Init(VkContextDevice &inDevice, const VkExtent2D &inExtent, Image &inImage)
    {
        Create(inDevice, inExtent);

        VkDevice         device          = LogicalDevice::GetDevice();
        const VkExtent3D drawImageExtent = {m_SwapchainExtent.width, m_SwapchainExtent.height, 1};

        constexpr VmaAllocationCreateInfo rImageAllocInfo = {.usage         = VMA_MEMORY_USAGE_GPU_ONLY,
                                                             .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        // Draw Image
        {   
            inImage.DrawImage.ImageFormat = m_SwapchainImageFormat;
            inImage.DrawImage.ImageExtent = drawImageExtent;

            VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            const VkImageCreateInfo rImageInfo = vkinit::image_create_info(inImage.DrawImage.ImageFormat, drawImageUsages, drawImageExtent);

            VK_CHECK(
                    vmaCreateImage(Allocator::GetAllocator(), &rImageInfo, &rImageAllocInfo, &inImage.DrawImage.Image, &inImage.DrawImage.Allocation, nullptr));

            const VkImageViewCreateInfo rViewInfo =
                    vkinit::imageview_create_info(inImage.DrawImage.ImageFormat, inImage.DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);

            VK_CHECK(vkCreateImageView(device, &rViewInfo, nullptr, &inImage.DrawImage.ImageView));
            std::cerr << "DrawImage.Image: " << inImage.DrawImage.Image << std::endl;
        }

        // Depth Image
        {
            inImage.DepthImage.ImageFormat = VK_FORMAT_D32_SFLOAT;
            inImage.DepthImage.ImageExtent = drawImageExtent;

            VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

            const VkImageCreateInfo dimgInfo = vkinit::image_create_info(inImage.DepthImage.ImageFormat, depthImageUsages, drawImageExtent);

            VK_CHECK(
                    vmaCreateImage(Allocator::GetAllocator(), &dimgInfo, &rImageAllocInfo, &inImage.DepthImage.Image, &inImage.DepthImage.Allocation, nullptr));

            const VkImageViewCreateInfo dViewInfo =
                    vkinit::imageview_create_info(inImage.DepthImage.ImageFormat, inImage.DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);

            VK_CHECK(vkCreateImageView(device, &dViewInfo, nullptr, &inImage.DepthImage.ImageView));
        }

    }

    void MSwapchain::Create(const VkContextDevice &inDevice, const VkExtent2D &inExtent)
    {
        vkb::SwapchainBuilder swapchainBuilder{inDevice.GetPhysicalDevice(), LogicalDevice::GetDevice(), inDevice.Surface};
        m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        const auto resultSwapchain =
                swapchainBuilder.set_desired_format(VkSurfaceFormatKHR{.format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                        .set_desired_extent(inExtent.width, inExtent.height)
                        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                        .build();

        if (!resultSwapchain.has_value())
        {
            throw std::runtime_error("Result swapchain no has in createSwapchain");
        }

        vkb::Swapchain vkbSwapchain = resultSwapchain.value();
        m_SwapchainExtent     = vkbSwapchain.extent;
        m_Swapchain           = vkbSwapchain.swapchain;
        m_SwapchainImages     = vkbSwapchain.get_images().value();
        m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
        m_SwapchainImageFormat      = vkbSwapchain.image_format;

    }

    std::pair<VkResult, uint32_t> MSwapchain::AcquireImage(VkDevice inDevice, VkSemaphore inSemaphore)
    {
        const VkResult result = vkAcquireNextImageKHR(inDevice, m_Swapchain, UINT64_MAX, inSemaphore, VK_NULL_HANDLE, &m_CurrentImageIndex);
        return {result, m_CurrentImageIndex};
    }

    VkResult MSwapchain::Present(VkQueue inQueue, const VkSemaphore *inRenderSemaphore, const uint32_t inImageIndex) const
    {
        const VkPresentInfoKHR presentInfo = vkinit::present_info(&m_Swapchain, 1, inRenderSemaphore, 1, &inImageIndex);

        const VkResult presentResult = vkQueuePresentKHR(inQueue, &presentInfo);

        return presentResult;
    }

    void MSwapchain::Destroy(VkDevice inDevice)
    {
        for (auto& view : m_SwapchainImageViews)
        {
            vkDestroyImageView(inDevice, view, nullptr);
        }
        m_SwapchainImageViews.clear();

        if (m_Swapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(inDevice, m_Swapchain, nullptr);

    }

}