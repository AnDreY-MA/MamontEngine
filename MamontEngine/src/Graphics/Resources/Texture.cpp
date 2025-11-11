#include "Graphics/Resources/Texture.h"
#include "Core/Engine.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"

namespace MamontEngine
{
    void Texture::Load(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped)
    {
        const size_t    dataSize     = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer = CreateStagingBuffer(dataSize);
        // uploadbuffer.Create(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(uploadbuffer.Info.pMappedData, inData, dataSize);

        Create(inSize, inFormat, inUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inIsMipMapped);

        const auto &contextDevice = MEngine::Get().GetContextDevice();
        contextDevice.ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkUtil::transition_image(cmd, Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset      = 0;
                    copyRegion.bufferRowLength   = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel       = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount     = 1;
                    copyRegion.imageExtent                     = inSize;

                    vkCmdCopyBufferToImage(cmd, uploadbuffer.Buffer, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                    if (inIsMipMapped)
                    {
                        VkUtil::generate_mipmaps(cmd, Image, VkExtent2D{ImageExtent.width, ImageExtent.height});
                    }
                    else
                    {
                        VkUtil::transition_image(cmd, Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    }
                });

        uploadbuffer.Destroy();
    }

    void Texture::Destroy()
    {
        const auto device = LogicalDevice::GetDevice();
        if (Sampler)
        {
            vkDestroySampler(device, Sampler, nullptr);
        }

        if (ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, ImageView, nullptr);
        }

        if (Image != VK_NULL_HANDLE && Allocation != nullptr)
        {
            vmaDestroyImage(Allocator::GetAllocator(), Image, Allocation);
        }
    }

    void Texture::Create(const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped, uint32_t arrayLayers)
    {
        ImageFormat           = inFormat;
        ImageExtent           = inSize;
        const VkDevice device = LogicalDevice::GetDevice();

        VkImageCreateInfo img_info = vkinit::image_create_info(inFormat, inUsage, inSize, arrayLayers);
        if (inIsMipMapped)
        {
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(inSize.width, inSize.height)))) + 1;
        }

        constexpr auto allocinfo =
                VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &img_info, &allocinfo, &Image, &Allocation, &Info));

        const VkImageAspectFlags aspectFlag =
                (inFormat == VK_FORMAT_D32_SFLOAT || inFormat == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        const VkImageViewType viewType = arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;

        uint32_t mipLevels = inIsMipMapped ? (img_info.mipLevels > 1 ? img_info.mipLevels : 1) : 1;

        /*if (mipLevels == 0)
        {
            mipLevels = 1;
        }*/

        const VkImageViewCreateInfo view_info = vkinit::imageviewCreateInfo(inFormat, Image, aspectFlag, mipLevels, arrayLayers, viewType);

        VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &ImageView));
    }

    Texture CreateWhiteTexture()
    {
        Texture        newTexture{};
        const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        newTexture.Load((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
        return newTexture;
    }

    Texture CreateErrorTexture()
    {
        Texture        newTexture{};
        const uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));

        const uint32_t                magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels{}; // for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }
        newTexture.Load(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
        return newTexture;
    }
} // namespace MamontEngine
