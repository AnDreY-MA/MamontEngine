#include "Graphics/Vulkan/Swapchain.h"

#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Core/ContextDevice.h"
#include "Utils//VkInitializers.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
//#include "Utils/Utilities.h"

namespace MamontEngine
{
    void MSwapchain::Init(VkContextDevice &inDevice, const VkExtent2D &inExtent)
    {
        Create(inDevice, inExtent);
    }

    void MSwapchain::Create(const VkContextDevice &inDevice, const VkExtent2D &inExtent)
    {
        vkb::SwapchainBuilder swapchainBuilder{PhysicalDevice::GetDevice(), LogicalDevice::GetDevice(), inDevice.Surface};
        m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM; /*VK_FORMAT_R16G16B16A16_SFLOAT*/
        /*VK_FORMAT_B8G8R8A8_UNORM*/
        //Utils::FindSupportedFormat(PhysicalDevice::GetDevice());
        
        const auto resultSwapchain =
                swapchainBuilder.set_desired_format(VkSurfaceFormatKHR{.format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT})
                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                        .set_desired_extent(inExtent.width, inExtent.height)
                        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
                        .build();

        if (!resultSwapchain.has_value())
        {
            throw std::runtime_error("Result swapchain no has in createSwapchain");
        }

        m_vkbSwapchain              = resultSwapchain.value();
        m_SwapchainExtent           = m_vkbSwapchain.extent;
        m_Swapchain                 = m_vkbSwapchain.swapchain;
        m_SwapchainImages           = m_vkbSwapchain.get_images().value();
        m_SwapchainImageViews       = m_vkbSwapchain.get_image_views().value();
    }

    void MSwapchain::ReCreate(const VkContextDevice &inDevice, const VkExtent2D &inExtent)
    {
        const VkDevice device = LogicalDevice::GetDevice();

        Destroy(device);

        Create(inDevice, inExtent);
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
        {
            vkDestroySwapchainKHR(inDevice, m_Swapchain, nullptr);
        }
    }
}