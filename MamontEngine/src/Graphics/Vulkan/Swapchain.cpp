#include "Graphics/Vulkan/Swapchain.h"
#include "Graphics/Vulkan/Image.h"
#include <VkBootstrap.h>
#include "Core/ContextDevice.h"
#include "Core/VkInitializers.h"

namespace MamontEngine
{
    void MSwapchain::Init(VkContextDevice &inDevice, const VkExtent2D& inExtent, Image &inImage)
    {
        Create(inDevice, inExtent.width, inExtent.height);

        const VkExtent3D drawImageExtent = {inExtent.width, inExtent.height, 1};

        inImage.DrawImage.ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        inImage.DrawImage.ImageExtent = drawImageExtent;

        VkImageUsageFlags drawImageUsages{};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        const VkImageCreateInfo rImageInfo = vkinit::image_create_info(inImage.DrawImage.ImageFormat, drawImageUsages, drawImageExtent);

        VmaAllocationCreateInfo rImageAllocInfo = {};
        rImageAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
        rImageAllocInfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vmaCreateImage(inDevice.Allocator, &rImageInfo, &rImageAllocInfo, &inImage.DrawImage.Image, &inImage.DrawImage.Allocation, nullptr);

        const VkImageViewCreateInfo rViewInfo =
                vkinit::imageview_create_info(inImage.DrawImage.ImageFormat, inImage.DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(inDevice.Device, &rViewInfo, nullptr, &inImage.DrawImage.ImageView));

        //> depthimg
        inImage.DepthImage.ImageFormat = VK_FORMAT_D32_SFLOAT;
        inImage.DepthImage.ImageExtent = drawImageExtent;
        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        const VkImageCreateInfo dimgInfo = vkinit::image_create_info(inImage.DepthImage.ImageFormat, depthImageUsages, drawImageExtent);
        vmaCreateImage(inDevice.Allocator, &dimgInfo, &rImageAllocInfo, &inImage.DepthImage.Image, &inImage.DepthImage.Allocation, nullptr);

        const VkImageViewCreateInfo dViewInfo =
                vkinit::imageview_create_info(inImage.DepthImage.ImageFormat, inImage.DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(inDevice.Device, &dViewInfo, nullptr, &inImage.DepthImage.ImageView));
    }

    void MSwapchain::Create(const VkContextDevice &inDevice, const uint32_t inWidth, const uint32_t inHeight)
    {
        vkb::SwapchainBuilder swapchainBuilder{inDevice.ChosenGPU, inDevice.Device, inDevice.Surface};
        m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        auto resultSwapchain =
                swapchainBuilder.set_desired_format(VkSurfaceFormatKHR{.format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                        .set_desired_extent(inWidth, inHeight)
                        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
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

        fmt::println("CreateSwapchain");
    }

    std::pair<VkResult, uint32_t> MSwapchain::AcquireImage(VkDevice inDevice, VkSemaphore &inSemaphore) const
    {
        uint32_t swapchainImageIndex;
        const VkResult result = vkAcquireNextImageKHR(inDevice, m_Swapchain, 1000000000, inSemaphore, nullptr, &swapchainImageIndex);
        return {result, swapchainImageIndex};
    }

    VkResult MSwapchain::Present(VkQueue inQueue, const VkSemaphore *inRenderSemaphore, const uint32_t inImageIndex)
    {
        VkPresentInfoKHR presentInfo = vkinit::present_info();
        presentInfo.pSwapchains      = &m_Swapchain;
        presentInfo.swapchainCount   = 1;

        presentInfo.pWaitSemaphores    = inRenderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &inImageIndex;

        const VkResult presentResult = vkQueuePresentKHR(inQueue, &presentInfo);

        return presentResult;
    }

    void MSwapchain::Destroy(VkDevice inDevice)
    {
        vkDestroySwapchainKHR(inDevice, m_Swapchain, nullptr);

        for (size_t i = 0; i < m_Framebuffers.size(); i++)
        {
            vkDestroyFramebuffer(inDevice, m_Framebuffers[i], nullptr);
        }

        for (int i = 0; i < m_SwapchainImageViews.size(); ++i)
        {
            vkDestroyImageView(inDevice, m_SwapchainImageViews[i], nullptr);
        }
    }

    void MSwapchain::CreateFrameBuffers(VkDevice inDevice, VkRenderPass inRenderPass, VkImageView inDepthImageView)
    {
        m_Framebuffers.resize(m_SwapchainImageViews.size());

        for (size_t i = 0; i < m_SwapchainImageViews.size(); i++)
        {
            std::array<VkImageView, 2> attachments = {m_SwapchainImageViews[i], inDepthImageView};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = inRenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments    = attachments.data();
            framebufferInfo.width           = m_SwapchainExtent.width;
            framebufferInfo.height          = m_SwapchainExtent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(inDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

}