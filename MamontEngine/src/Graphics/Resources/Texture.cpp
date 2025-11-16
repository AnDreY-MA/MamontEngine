#include "Graphics/Resources/Texture.h"
#include "Core/Engine.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include "ktx.h"
#include "ktxvulkan.h"
#include "stb_image.h"
#include "Graphics/Vulkan/ImmediateContext.h"

namespace MamontEngine
{
    void Texture::Load(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped)
    {
        const size_t    dataSize     = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer = CreateStagingBuffer(dataSize);

        memcpy(uploadbuffer.Info.pMappedData, inData, dataSize);

        Create(inSize, inFormat, inUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inIsMipMapped);

        const auto &contextDevice = MEngine::Get().GetContextDevice();
        ImmediateContext::ImmediateSubmit(
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

    void Texture::Create(const VkExtent3D  &inSize,
                         VkFormat           inFormat,
                         VkImageUsageFlags  inUsage,
                         const bool         inIsMipMapped,
                         uint32_t           arrayLayers,
                         VkImageCreateFlags inCreateFlags)
    {
        ImageFormat           = inFormat;
        ImageExtent           = inSize;
        const VkDevice device = LogicalDevice::GetDevice();

        VkImageCreateInfo img_info = vkinit::image_create_info(inFormat, inUsage, inSize, arrayLayers);
        img_info.flags             = inCreateFlags;

        if (inIsMipMapped)
        {
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(inSize.width, inSize.height)))) + 1;
        }

        constexpr auto allocinfo =
                VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &img_info, &allocinfo, &Image, &Allocation, &Info));

        const VkImageAspectFlags aspectFlag =
                (inFormat == VK_FORMAT_D32_SFLOAT || inFormat == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageViewType viewType = arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        if (inCreateFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
        {
            viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }

        uint32_t mipLevels = inIsMipMapped ? (img_info.mipLevels > 1 ? img_info.mipLevels : 1) : 1;

        /*if (mipLevels == 0)
        {
            mipLevels = 1;
        }*/

        const VkImageViewCreateInfo view_info = vkinit::imageviewCreateInfo(inFormat, Image, aspectFlag, mipLevels, arrayLayers, viewType);

        VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &ImageView));
    }

    Texture LoadCubeMapTexture(const std::string &inFileName, VkFormat inFormat)
    {
        ktxTexture *cubeMapTexture = nullptr;

        auto result = ktxTexture_CreateFromNamedFile(inFileName.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &cubeMapTexture);
        assert(result == KTX_SUCCESS);

        const VkExtent3D extentSize = {
                .width = static_cast<uint32_t>(cubeMapTexture->baseWidth), .height = static_cast<uint32_t>(cubeMapTexture->baseHeight), .depth = 1};

        const uint32_t mipLevels      = cubeMapTexture->numLevels;
        ktx_uint8_t   *ktxTextureData = ktxTexture_GetData(cubeMapTexture);
        ktx_size_t     ktxTextureSize = ktxTexture_GetDataSize(cubeMapTexture);


        AllocatedBuffer uploadBuffer = CreateStagingBuffer(ktxTextureSize);
        memcpy(uploadBuffer.Info.pMappedData, ktxTextureData, ktxTextureSize);

        Texture newTexture{};

        // Create CubeMapTexture
        {
            newTexture.ImageFormat = inFormat;
            newTexture.ImageExtent = extentSize;

            VkImageCreateInfo imgInfo = vkinit::image_create_info(inFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extentSize, 6);
            imgInfo.mipLevels         = mipLevels;
            imgInfo.flags             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            constexpr VmaAllocationCreateInfo allocInfo = {.usage         = VMA_MEMORY_USAGE_GPU_ONLY,
                                                           .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

            VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imgInfo, &allocInfo, &newTexture.Image, &newTexture.Allocation, &newTexture.Info));

            VkImageViewCreateInfo viewInfo =
                    vkinit::imageviewCreateInfo(inFormat, newTexture.Image, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6, VK_IMAGE_VIEW_TYPE_CUBE);

            VK_CHECK(vkCreateImageView(LogicalDevice::GetDevice(), &viewInfo, nullptr, &newTexture.ImageView));
        }

        auto &contextDevice = MEngine::Get().GetContextDevice();
        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    std::vector<VkBufferImageCopy> bufferCopyRegions;

                    for (uint32_t face = 0; face < 6; face++)
                    {
                        for (uint32_t level = 0; level < mipLevels; level++)
                        {
                            ktx_size_t     faceOffset = 0;
                            KTX_error_code ret        = ktxTexture_GetImageOffset(cubeMapTexture, level, 0, face, &faceOffset);
                            if (ret != KTX_SUCCESS)
                            {
                                throw std::runtime_error("Failed to get KTX image offset");
                            }

                            uint32_t mipWidth  = std::max(1u, cubeMapTexture->baseWidth >> level);
                            uint32_t mipHeight = std::max(1u, cubeMapTexture->baseHeight >> level);

                            VkBufferImageCopy bufferCopyRegion               = {};
                            bufferCopyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                            bufferCopyRegion.imageSubresource.mipLevel       = level;
                            bufferCopyRegion.imageSubresource.baseArrayLayer = face;
                            bufferCopyRegion.imageSubresource.layerCount     = 1;
                            bufferCopyRegion.imageExtent.width               = mipWidth;
                            bufferCopyRegion.imageExtent.height              = mipHeight;
                            bufferCopyRegion.imageExtent.depth               = 1;
                            bufferCopyRegion.bufferOffset                    = faceOffset;

                            bufferCopyRegion.bufferRowLength   = 0;
                            bufferCopyRegion.bufferImageHeight = 0;

                            bufferCopyRegions.push_back(bufferCopyRegion);

                        }
                    }

                    VkUtil::transition_image(cmd, newTexture.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, 6);

                    vkCmdCopyBufferToImage(cmd,
                                           uploadBuffer.Buffer,
                                           newTexture.Image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           static_cast<uint32_t>(bufferCopyRegions.size()),
                                           bufferCopyRegions.data());

                    VkUtil::transition_image(
                            cmd, newTexture.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, 6);
                });

        VkSamplerCreateInfo samplerInfo = {};
        samplerInfo.sType               = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter           = VK_FILTER_LINEAR;
        samplerInfo.minFilter           = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipLodBias          = 0.0f;
        samplerInfo.compareOp           = VK_COMPARE_OP_NEVER;
        samplerInfo.minLod              = 0.0f;
        samplerInfo.maxLod              = static_cast<float>(mipLevels);
        samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        samplerInfo.maxAnisotropy       = 1.0f;

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(PhysicalDevice::GetDevice(), &deviceFeatures);
        if (deviceFeatures.samplerAnisotropy)
        {
            VkPhysicalDeviceProperties deviceProps;
            vkGetPhysicalDeviceProperties(PhysicalDevice::GetDevice(), &deviceProps);
            samplerInfo.maxAnisotropy    = deviceProps.limits.maxSamplerAnisotropy;
            samplerInfo.anisotropyEnable = VK_TRUE;
        }

        VK_CHECK(vkCreateSampler(LogicalDevice::GetDevice(), &samplerInfo, nullptr, &newTexture.Sampler));

        uploadBuffer.Destroy();
        ktxTexture_Destroy(cubeMapTexture);

        return newTexture;
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
