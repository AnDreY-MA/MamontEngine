#include "Graphics/Resources/Texture.h"
#include "Core/Engine.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Graphics/Vulkan/ImmediateContext.h"
#include "Utils/Utilities.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include "Utils/VkPipelines.h"
#include "fastgltf/types.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "ktx.h"
#include "stb_image.h"
#include "vulkan/vulkan_core.h"

#define M_PI 3.1415926535897932384626433832795

namespace MamontEngine
{
    void Texture::Load(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped, VkSampler inSampler)
    {
        const size_t    dataSize     = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer = CreateStagingBuffer(dataSize);

        memcpy(uploadbuffer.Info.pMappedData, inData, dataSize);

        Create(inSize, inFormat, inUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inIsMipMapped, 1, 0, inSampler);

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

        if (Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, Sampler, nullptr);
        }

        if (ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, ImageView, nullptr);
        }

        if (Image != VK_NULL_HANDLE)
        {
            vmaDestroyImage(Allocator::GetAllocator(), Image, Allocation);
        }

    }

    void Texture::Create(const VkExtent3D  &inSize,
                         VkFormat           inFormat,
                         VkImageUsageFlags  inUsage,
                         const bool         inIsMipMapped,
                         uint32_t           arrayLayers,
                         VkImageCreateFlags inCreateFlags,
                         VkSampler          inSampler)
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

        std::cerr << "New TEXTURE: " << Image << std::endl;

        const VkImageAspectFlags aspectFlag =
                (inFormat == VK_FORMAT_D32_SFLOAT || inFormat == VK_FORMAT_D24_UNORM_S8_UINT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

        VkImageViewType viewType = arrayLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        if (inCreateFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
        {
            viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        }

        uint32_t mipLevels = inIsMipMapped ? (img_info.mipLevels > 1 ? img_info.mipLevels : 1) : 1;

        const VkImageViewCreateInfo view_info = vkinit::imageviewCreateInfo(inFormat, Image, aspectFlag, mipLevels, arrayLayers, viewType);

        VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &ImageView));

        if (inSampler == VK_NULL_HANDLE)
        {
            VkSamplerCreateInfo sampl = {
                    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr, .magFilter = VK_FILTER_LINEAR, .minFilter = VK_FILTER_LINEAR};

            vkCreateSampler(device, &sampl, nullptr, &Sampler);
        }
    }

    Texture* LoadCubeMapTexture(const std::string &inFileName, VkFormat inFormat)
    {
        ktxTexture *cubeMapTexture = nullptr;

        auto result = ktxTexture_CreateFromNamedFile(inFileName.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &cubeMapTexture);
        assert(result == KTX_SUCCESS);

        if (cubeMapTexture->numFaces != 6)
        {
            ktxTexture_Destroy(cubeMapTexture);
            throw std::runtime_error("Texture is not a cubemap (expected 6 faces, got " + std::to_string(cubeMapTexture->numFaces) + ")");
        }

        const VkExtent3D extentSize = {
                .width = static_cast<uint32_t>(cubeMapTexture->baseWidth), .height = static_cast<uint32_t>(cubeMapTexture->baseHeight), .depth = 1};

        const uint32_t     mipLevels      = cubeMapTexture->numLevels;
        const ktx_uint8_t *ktxTextureData = ktxTexture_GetData(cubeMapTexture);
        const ktx_size_t   ktxTextureSize = ktxTexture_GetDataSize(cubeMapTexture);


        AllocatedBuffer uploadBuffer = CreateStagingBuffer(ktxTextureSize);
        memcpy(uploadBuffer.Info.pMappedData, ktxTextureData, ktxTextureSize);

        Texture* newTexture = new Texture();

        // Create CubeMapTexture
        {
            newTexture->ImageFormat = inFormat;
            newTexture->ImageExtent = extentSize;

            VkImageCreateInfo imgInfo = vkinit::image_create_info(inFormat, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, extentSize, 6);
            imgInfo.mipLevels         = mipLevels;
            imgInfo.flags             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            constexpr VmaAllocationCreateInfo allocInfo = {.usage         = VMA_MEMORY_USAGE_GPU_ONLY,
                                                           .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

            VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imgInfo, &allocInfo, &newTexture->Image, &newTexture->Allocation, &newTexture->Info));

            VkImageViewCreateInfo viewInfo =
                    vkinit::imageviewCreateInfo(inFormat, newTexture->Image, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels, 6, VK_IMAGE_VIEW_TYPE_CUBE);

            VK_CHECK(vkCreateImageView(LogicalDevice::GetDevice(), &viewInfo, nullptr, &newTexture->ImageView));
        }

        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    std::vector<VkBufferImageCopy> bufferCopyRegions;
                    bufferCopyRegions.reserve(cubeMapTexture->numFaces * mipLevels);

                    for (uint32_t face = 0; face < cubeMapTexture->numFaces; face++)
                    {
                        for (uint32_t level = 0; level < mipLevels; level++)
                        {
                            ktx_size_t           faceOffset = 0;
                            const KTX_error_code ret        = ktxTexture_GetImageOffset(cubeMapTexture, level, 0, face, &faceOffset);
                            if (ret != KTX_SUCCESS)
                            {
                                throw std::runtime_error("Failed to get KTX image offset");
                            }

                            const uint32_t mipWidth  = std::max(1u, cubeMapTexture->baseWidth >> level);
                            const uint32_t mipHeight = std::max(1u, cubeMapTexture->baseHeight >> level);

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

                    VkUtil::transition_image(cmd, newTexture->Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels, 6);

                    vkCmdCopyBufferToImage(cmd,
                                           uploadBuffer.Buffer,
                                           newTexture->Image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           static_cast<uint32_t>(bufferCopyRegions.size()),
                                           bufferCopyRegions.data());

                    VkUtil::transition_image(
                            cmd, newTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels, 6);
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

        VK_CHECK(vkCreateSampler(LogicalDevice::GetDevice(), &samplerInfo, nullptr, &newTexture->Sampler));

        uploadBuffer.Destroy();
        ktxTexture_Destroy(cubeMapTexture);

        return newTexture;
    }

    std::shared_ptr<Texture> CreateWhiteTexture()
    {
        std::shared_ptr<Texture> newTexture = std::make_shared<Texture>();
        const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        newTexture->Load((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
        return newTexture;
    }

    Texture* CreateErrorTexture()
    {
        Texture*        newTexture = new Texture();
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
        newTexture->Load(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
        return newTexture;
    }

    Texture *
    GeneratePrefilteredCube(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &inDrawSkyboxFunc, const Texture &inEnvironmentTexture)
    {
        const VkDevice       device  = LogicalDevice::GetDevice();
        VkFormat             format  = VK_FORMAT_R16G16B16A16_SFLOAT;
        VkImageUsageFlags    usage   = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        constexpr int32_t    dim     = 512;
        const uint32_t   numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
        constexpr VkExtent3D extent{.width = dim, .height = dim, .depth = 1};

        Texture *newTexture = new Texture();
        newTexture->ImageExtent                      = extent;
        newTexture->ImageFormat                      = format;
        VkImageCreateInfo imageInfo                 = vkinit::image_create_info(format, usage, extent);
        imageInfo.mipLevels                         = numMips;
        imageInfo.arrayLayers                       = 6;
        imageInfo.samples                           = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling                            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.flags                             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        constexpr VmaAllocationCreateInfo allocInfo = {
            .usage         = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imageInfo, &allocInfo, &newTexture->Image, &newTexture->Allocation, &newTexture->Info));

        VkImageViewCreateInfo viewCreateInfo       = vkinit::imageviewCreateInfo(format, newTexture->Image, VK_IMAGE_ASPECT_COLOR_BIT);
        viewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCreateInfo.subresourceRange.levelCount = numMips;
        viewCreateInfo.subresourceRange.layerCount = 6;
        VK_CHECK(vkCreateImageView(device, &viewCreateInfo, nullptr, &newTexture->ImageView));

        VkSamplerCreateInfo samplerCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        samplerCreateInfo.magFilter     = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter     = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.minLod        = 0.0f;
        samplerCreateInfo.maxLod        = static_cast<float>(numMips);
        samplerCreateInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &newTexture->Sampler));

        VkAttachmentDescription attDesc = {};
        // Color attachment
        attDesc.format                                 = format;
        attDesc.samples                                = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp                                 = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp                                = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp                          = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp                         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout                          = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout                            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        constexpr VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments    = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .pNext = nullptr};
        renderPassInfo.attachmentCount        = 1;
        renderPassInfo.pAttachments           = &attDesc;
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpassDescription;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = dependencies.data();
        VkRenderPass renderPass;
        VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        struct
        {
            VkImage        Image{VK_NULL_HANDLE};
            VkImageView    View{VK_NULL_HANDLE};
            VkDeviceMemory Memory{VK_NULL_HANDLE};
            VkFramebuffer  Framebuffer{VK_NULL_HANDLE};
        } offscreen{};
        // Offscreen framebuffer
        {
            VkImageCreateInfo imageInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, extent);
            imageInfo.mipLevels         = 1;
            imageInfo.arrayLayers       = 1;
            imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
            VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &offscreen.Image));

            std::cerr << "------"
                      << "OFFSCREEN, PREFILT, IMAGE: " << offscreen.Image << "\n";

            VkMemoryAllocateInfo memAlloc = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = nullptr};
            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(device, offscreen.Image, &memReqs);
            memAlloc.allocationSize  = memReqs.size;
            memAlloc.memoryTypeIndex = Utils::GetMemoryType(PhysicalDevice::GetDevice(), memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VK_CHECK(vkAllocateMemory(device, &memAlloc, nullptr, &offscreen.Memory));
            VK_CHECK(vkBindImageMemory(device, offscreen.Image, offscreen.Memory, 0));

            VkImageViewCreateInfo colorImageView           = vkinit::imageviewCreateInfo(format, offscreen.Image, VK_IMAGE_ASPECT_COLOR_BIT);
            colorImageView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            colorImageView.flags                           = 0;
            colorImageView.subresourceRange                = {};
            colorImageView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            colorImageView.subresourceRange.baseMipLevel   = 0;
            colorImageView.subresourceRange.levelCount     = 1;
            colorImageView.subresourceRange.baseArrayLayer = 0;
            colorImageView.subresourceRange.layerCount     = 1;
            VK_CHECK(vkCreateImageView(device, &colorImageView, nullptr, &offscreen.View));

            VkFramebufferCreateInfo framebufferInfo = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .pNext = nullptr};
            framebufferInfo.renderPass              = renderPass;
            framebufferInfo.attachmentCount         = 1;
            framebufferInfo.pAttachments            = &offscreen.View;
            framebufferInfo.width                   = dim;
            framebufferInfo.height                  = dim;
            framebufferInfo.layers                  = 1;
            VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &offscreen.Framebuffer));

            ImmediateContext::ImmediateSubmit(
                    [&](VkCommandBuffer cmd)
                    { VkUtil::transition_image(cmd, offscreen.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); });
        }

        const std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {VkDescriptorSetLayoutBinding{
                .binding        = 0,    
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, 
                }
        };

        VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                                      .pNext        = nullptr,
                                                                      .bindingCount = static_cast<uint32_t>(setLayoutBindings.size()),
                                                                      .pBindings    = setLayoutBindings.data()};
        VkDescriptorSetLayout           descirptorSetLayout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descirptorSetLayout));

        constexpr std::array<VkDescriptorPoolSize, 1> poolSizes = {VkDescriptorPoolSize{
                .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
        }};

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr};
        descriptorPoolInfo.pPoolSizes                 = poolSizes.data();
        descriptorPoolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.maxSets                    = 2;

        VkDescriptorPool descriptorPool;
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

        VkDescriptorSet             descriptorSet;
        VkDescriptorSetAllocateInfo descriptorSetallocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr};
        descriptorSetallocInfo.descriptorSetCount          = 1;
        descriptorSetallocInfo.descriptorPool              = descriptorPool;
        descriptorSetallocInfo.pSetLayouts                 = &descirptorSetLayout;
        VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetallocInfo, &descriptorSet));
        VkWriteDescriptorSet writeDescriptorSet = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr};
        writeDescriptorSet.dstSet               = descriptorSet;
        writeDescriptorSet.descriptorType       = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.dstBinding             = 0;
        writeDescriptorSet.dstArrayElement        = 0;
        writeDescriptorSet.descriptorCount        = 1;
        const VkDescriptorImageInfo descImageInfo     = {
                    .sampler     = inEnvironmentTexture.Sampler,
                    .imageView   = inEnvironmentTexture.ImageView, 
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 
        };

        writeDescriptorSet.pImageInfo = &descImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

        struct PushBlock
        {
            glm::mat4 MVP;
            VkDeviceAddress VertexBuffer{0};
            float     Roughness  = 0.f;
            uint32_t  NumSamples = 32u;
        } pushBlock;
        pushBlock.VertexBuffer = vertexAddress;

        VkPipelineLayout                         pipelineLayout = VK_NULL_HANDLE;
        const std::array<VkPushConstantRange, 1> pustConstantRanges = {
                VkPushConstantRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0, 
                .size = sizeof(PushBlock), 
                }};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        pipelineLayoutInfo.pSetLayouts                = &descirptorSetLayout;
        pipelineLayoutInfo.setLayoutCount             = 1;
        pipelineLayoutInfo.pushConstantRangeCount     = 1;
        pipelineLayoutInfo.pPushConstantRanges        = pustConstantRanges.data();
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetVertexInput(vertexInputInfo);

        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.EnableDepthClamp(VK_FALSE);
        pipelineBuilder.m_Multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineBuilder.m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineBuilder.m_Multisampling.flags                = 0;
        pipelineBuilder.SetLayout(pipelineLayout);
        // pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
        // pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR);
        pipelineBuilder.m_Rasterizer.lineWidth = 1.0f;

        const std::string prefilterenvmapath = DEFAULT_ASSETS_DIRECTORY + "Shaders/prefilterenvmap.frag.spv";

        VkShaderModule prefilterenvmapFragShader;
        if (!VkPipelines::LoadShaderModule(prefilterenvmapath.c_str(), device, &prefilterenvmapFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string filtercubeertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/filtercube.vert.spv";
        VkShaderModule    filtercubeVertexShader;
        if (!VkPipelines::LoadShaderModule(filtercubeertexShaderPath.c_str(), device, &filtercubeVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }
        pipelineBuilder.SetShaders(filtercubeVertexShader, prefilterenvmapFragShader);

        VkPipeline pipeline = pipelineBuilder.BuildPipline(device, 1, renderPass);

        vkDestroyShaderModule(device, prefilterenvmapFragShader, nullptr);
        vkDestroyShaderModule(device, filtercubeVertexShader, nullptr);
        
        VkClearValue clearValues[1]{};
        clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo    = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .pNext = nullptr};
        renderPassBeginInfo.renderPass               = renderPass;
        renderPassBeginInfo.framebuffer              = offscreen.Framebuffer;
        renderPassBeginInfo.renderArea.extent.width  = dim;
        renderPassBeginInfo.renderArea.extent.height = dim;
        renderPassBeginInfo.clearValueCount          = 1;
        renderPassBeginInfo.pClearValues             = clearValues;

        const std::array<glm::mat4, 6> matrices = {
                // POSITIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };
        VkViewport viewport = {.width = (float)dim, .height = (float)dim, .minDepth = 0, .maxDepth = 1.0f};
        VkRect2D   scissor{};
        scissor.extent.width  = dim;
        scissor.extent.height = dim;
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;

        VkImageSubresourceRange subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = numMips;
        subresourceRange.layerCount              = 6;

        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    vkCmdSetViewport(cmd, 0, 1, &viewport);
                    vkCmdSetScissor(cmd, 0, 1, &scissor);
                    VkUtil::transition_image(cmd, newTexture->Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numMips, 6);

                    for (uint32_t m = 0; m < numMips; m++)
                    {
                        pushBlock.Roughness = static_cast<float>(m) / static_cast<float>(numMips - 1);
                        for (uint32_t f = 0; f < 6; f++)
                        {
                            viewport.width  = static_cast<float>(dim * std::pow(0.5f, m));
                            viewport.height = static_cast<float>(dim * std::pow(0.5f, m));

                            vkCmdSetViewport(cmd, 0, 1, &viewport);

                            vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                            pushBlock.MVP = glm::perspective((float)(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

                            vkCmdPushConstants(
                                    cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

                            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
                            //--- Function Draw Skybox
                            inDrawSkyboxFunc(cmd);
                            vkCmdEndRenderPass(cmd);

                            VkUtil::transition_image(cmd, offscreen.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                            VkImageCopy copyRegion = {};

                            copyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                            copyRegion.srcSubresource.baseArrayLayer = 0;
                            copyRegion.srcSubresource.mipLevel       = 0;
                            copyRegion.srcSubresource.layerCount     = 1;
                            copyRegion.srcOffset                     = {0, 0, 0};

                            copyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                            copyRegion.dstSubresource.baseArrayLayer = f;
                            copyRegion.dstSubresource.mipLevel       = m;
                            copyRegion.dstSubresource.layerCount     = 1;
                            copyRegion.dstOffset                     = {0, 0, 0};

                            copyRegion.extent.width  = static_cast<uint32_t>(viewport.width);
                            copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                            copyRegion.extent.depth  = 1;

                            vkCmdCopyImage(cmd,
                                           offscreen.Image,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           newTexture->Image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           1,
                                           &copyRegion);


                            VkUtil::transition_image(cmd, offscreen.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                        }
                    }

                    VkUtil::transition_image(cmd, newTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, numMips, 6);
                });

        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyFramebuffer(device, offscreen.Framebuffer, nullptr);
        vkFreeMemory(device, offscreen.Memory, nullptr);
        vkDestroyImageView(device, offscreen.View, nullptr);
        vkDestroyImage(device, offscreen.Image, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descirptorSetLayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        return newTexture;
    }
    
    Texture* GenerateIrradianceTexture(VkDeviceAddress                           vertexAddress,
        std::function<void(VkCommandBuffer cmd)>& inDrawSkyboxFunc,
        const Texture& inEnvironmentTexture)
    {
        const VkDevice       device  = LogicalDevice::GetDevice();
        VkFormat             format  = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkImageUsageFlags    usage   = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        constexpr int32_t    dim     = 64;
        const uint32_t       numMips = static_cast<uint32_t>(floor(log2(dim))) + 1;
        constexpr VkExtent3D extent{.width = dim, .height = dim, .depth = 1};

        Texture* newTexture = new Texture();
        newTexture->ImageExtent                      = extent;
        newTexture->ImageFormat                      = format;
        VkImageCreateInfo imageInfo                 = vkinit::image_create_info(format, usage, extent);
        imageInfo.mipLevels                         = numMips;
        imageInfo.arrayLayers                       = 6;
        imageInfo.samples                           = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling                            = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.flags                             = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        constexpr VmaAllocationCreateInfo allocInfo = {.usage         = VMA_MEMORY_USAGE_GPU_ONLY,
                                                       .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imageInfo, &allocInfo, &newTexture->Image, &newTexture->Allocation, &newTexture->Info));

        VkImageViewCreateInfo viewCreateInfo       = vkinit::imageviewCreateInfo(format, newTexture->Image, VK_IMAGE_ASPECT_COLOR_BIT);
        viewCreateInfo.viewType                    = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCreateInfo.subresourceRange.levelCount = numMips;
        viewCreateInfo.subresourceRange.layerCount = 6;
        VK_CHECK(vkCreateImageView(device, &viewCreateInfo, nullptr, &newTexture->ImageView));

        VkSamplerCreateInfo samplerCreateInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
        samplerCreateInfo.magFilter           = VK_FILTER_LINEAR;
        samplerCreateInfo.minFilter           = VK_FILTER_LINEAR;
        samplerCreateInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerCreateInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerCreateInfo.minLod              = 0.0f;
        samplerCreateInfo.maxLod              = static_cast<float>(numMips);
        samplerCreateInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK(vkCreateSampler(device, &samplerCreateInfo, nullptr, &newTexture->Sampler));

        VkAttachmentDescription attDesc = {};
        // Color attachment
        attDesc.format                                 = format;
        attDesc.samples                                = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp                                 = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp                                = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp                          = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp                         = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout                          = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout                            = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        constexpr VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments    = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .pNext = nullptr};
        renderPassInfo.attachmentCount        = 1;
        renderPassInfo.pAttachments           = &attDesc;
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpassDescription;
        renderPassInfo.dependencyCount        = 2;
        renderPassInfo.pDependencies          = dependencies.data();
        VkRenderPass renderPass;
        VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));

        struct
        {
            VkImage        Image{VK_NULL_HANDLE};
            VkImageView    View{VK_NULL_HANDLE};
            VkDeviceMemory Memory{VK_NULL_HANDLE};
            VkFramebuffer  Framebuffer{VK_NULL_HANDLE};
        } offscreen{};
        // Offscreen framebuffer
        {
            VkImageCreateInfo imageInfo = vkinit::image_create_info(format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, extent);
            imageInfo.mipLevels         = 1;
            imageInfo.arrayLayers       = 1;
            imageInfo.samples           = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.tiling            = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
            VK_CHECK(vkCreateImage(device, &imageInfo, nullptr, &offscreen.Image));

            std::cerr << "------"
                      << "OFFSCREEN, PREFILT, IMAGE: " << offscreen.Image << "\n";

            VkMemoryAllocateInfo memAlloc = {.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, .pNext = nullptr};
            VkMemoryRequirements memReqs;
            vkGetImageMemoryRequirements(device, offscreen.Image, &memReqs);
            memAlloc.allocationSize  = memReqs.size;
            memAlloc.memoryTypeIndex = Utils::GetMemoryType(PhysicalDevice::GetDevice(), memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            VK_CHECK(vkAllocateMemory(device, &memAlloc, nullptr, &offscreen.Memory));
            VK_CHECK(vkBindImageMemory(device, offscreen.Image, offscreen.Memory, 0));

            VkImageViewCreateInfo colorImageView           = vkinit::imageviewCreateInfo(format, offscreen.Image, VK_IMAGE_ASPECT_COLOR_BIT);
            colorImageView.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
            colorImageView.flags                           = 0;
            colorImageView.subresourceRange                = {};
            colorImageView.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            colorImageView.subresourceRange.baseMipLevel   = 0;
            colorImageView.subresourceRange.levelCount     = 1;
            colorImageView.subresourceRange.baseArrayLayer = 0;
            colorImageView.subresourceRange.layerCount     = 1;
            VK_CHECK(vkCreateImageView(device, &colorImageView, nullptr, &offscreen.View));

            VkFramebufferCreateInfo framebufferInfo = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .pNext = nullptr};
            framebufferInfo.renderPass              = renderPass;
            framebufferInfo.attachmentCount         = 1;
            framebufferInfo.pAttachments            = &offscreen.View;
            framebufferInfo.width                   = dim;
            framebufferInfo.height                  = dim;
            framebufferInfo.layers                  = 1;
            VK_CHECK(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &offscreen.Framebuffer));

            ImmediateContext::ImmediateSubmit(
                    [&](VkCommandBuffer cmd)
                    { VkUtil::transition_image(cmd, offscreen.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); });
        }

        const std::array<VkDescriptorSetLayoutBinding, 1> setLayoutBindings = {VkDescriptorSetLayoutBinding{
                .binding         = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT,
        }};

        VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                                      .pNext        = nullptr,
                                                                      .bindingCount = static_cast<uint32_t>(setLayoutBindings.size()),
                                                                      .pBindings    = setLayoutBindings.data()};
        VkDescriptorSetLayout           descirptorSetLayout;
        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorLayoutCreateInfo, nullptr, &descirptorSetLayout));

        constexpr std::array<VkDescriptorPoolSize, 1> poolSizes = {VkDescriptorPoolSize{
                .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
        }};

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr};
        descriptorPoolInfo.pPoolSizes                 = poolSizes.data();
        descriptorPoolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.maxSets                    = 2;

        VkDescriptorPool descriptorPool;
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

        VkDescriptorSet             descriptorSet;
        VkDescriptorSetAllocateInfo descriptorSetallocInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr};
        descriptorSetallocInfo.descriptorSetCount          = 1;
        descriptorSetallocInfo.descriptorPool              = descriptorPool;
        descriptorSetallocInfo.pSetLayouts                 = &descirptorSetLayout;
        VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetallocInfo, &descriptorSet));
        VkWriteDescriptorSet writeDescriptorSet   = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr};
        writeDescriptorSet.dstSet                 = descriptorSet;
        writeDescriptorSet.descriptorType         = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeDescriptorSet.dstBinding             = 0;
        writeDescriptorSet.dstArrayElement        = 0;
        writeDescriptorSet.descriptorCount        = 1;
        const VkDescriptorImageInfo descImageInfo = {
                .sampler     = inEnvironmentTexture.Sampler,
                .imageView   = inEnvironmentTexture.ImageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        writeDescriptorSet.pImageInfo = &descImageInfo;
        vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);

        struct PushBlock
        {
            glm::mat4       MVP;
            VkDeviceAddress VertexBuffer{0};
            float           DeltaPhi   = (2.0f * float(M_PI)) / 180.0f;
            float           DeltaTheta = (0.5f * float(M_PI)) / 64.0f;
        } pushBlock;
        pushBlock.VertexBuffer = vertexAddress;

        VkPipelineLayout                         pipelineLayout     = VK_NULL_HANDLE;
        const std::array<VkPushConstantRange, 1> pustConstantRanges = {VkPushConstantRange{
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset     = 0,
                .size       = sizeof(PushBlock),
        }};
        VkPipelineLayoutCreateInfo               pipelineLayoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        pipelineLayoutInfo.pSetLayouts                              = &descirptorSetLayout;
        pipelineLayoutInfo.setLayoutCount                           = 1;
        pipelineLayoutInfo.pushConstantRangeCount                   = 1;
        pipelineLayoutInfo.pPushConstantRanges                      = pustConstantRanges.data();
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetVertexInput(vertexInputInfo);

        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.EnableDepthClamp(VK_FALSE);
        pipelineBuilder.m_Multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineBuilder.m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineBuilder.m_Multisampling.flags                = 0;
        pipelineBuilder.SetLayout(pipelineLayout);
        pipelineBuilder.m_Rasterizer.lineWidth = 1.0f;

        const std::string prefilterenvmapath = DEFAULT_ASSETS_DIRECTORY + "Shaders/irradiancecube.frag.spv";

        VkShaderModule prefilterenvmapFragShader;
        if (!VkPipelines::LoadShaderModule(prefilterenvmapath.c_str(), device, &prefilterenvmapFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string filtercubeertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/filtercube.vert.spv";
        VkShaderModule    filtercubeVertexShader;
        if (!VkPipelines::LoadShaderModule(filtercubeertexShaderPath.c_str(), device, &filtercubeVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }
        pipelineBuilder.SetShaders(filtercubeVertexShader, prefilterenvmapFragShader);

        VkPipeline pipeline = pipelineBuilder.BuildPipline(device, 1, renderPass);

        vkDestroyShaderModule(device, prefilterenvmapFragShader, nullptr);
        vkDestroyShaderModule(device, filtercubeVertexShader, nullptr);

        VkClearValue clearValues[1]{};
        clearValues[0].color = {{0.0f, 0.0f, 0.2f, 0.0f}};

        VkRenderPassBeginInfo renderPassBeginInfo    = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .pNext = nullptr};
        renderPassBeginInfo.renderPass               = renderPass;
        renderPassBeginInfo.framebuffer              = offscreen.Framebuffer;
        renderPassBeginInfo.renderArea.extent.width  = dim;
        renderPassBeginInfo.renderArea.extent.height = dim;
        renderPassBeginInfo.clearValueCount          = 1;
        renderPassBeginInfo.pClearValues             = clearValues;

        const std::array<glm::mat4, 6> matrices = {
                // POSITIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_X
                glm::rotate(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Y
                glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // POSITIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                // NEGATIVE_Z
                glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        };
        VkViewport viewport = {.width = (float)dim, .height = (float)dim, .minDepth = 0, .maxDepth = 1.0f};
        VkRect2D   scissor{};
        scissor.extent.width  = dim;
        scissor.extent.height = dim;
        scissor.offset.x      = 0;
        scissor.offset.y      = 0;

        VkImageSubresourceRange subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT};
        subresourceRange.baseMipLevel            = 0;
        subresourceRange.levelCount              = numMips;
        subresourceRange.layerCount              = 6;

        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    vkCmdSetViewport(cmd, 0, 1, &viewport);
                    vkCmdSetScissor(cmd, 0, 1, &scissor);
                    VkUtil::transition_image(cmd, newTexture->Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, numMips, 6);

                    for (uint32_t m = 0; m < numMips; m++)
                    {
                        for (uint32_t f = 0; f < 6; f++)
                        {
                            viewport.width  = static_cast<float>(dim * std::pow(0.5f, m));
                            viewport.height = static_cast<float>(dim * std::pow(0.5f, m));

                            vkCmdSetViewport(cmd, 0, 1, &viewport);

                            vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

                            pushBlock.MVP = glm::perspective(static_cast<float>(M_PI / 2.0), 1.0f, 0.1f, 512.0f) * matrices[f];

                            vkCmdPushConstants(
                                    cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushBlock), &pushBlock);

                            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, NULL);
                            //--- Function Draw Skybox
                            inDrawSkyboxFunc(cmd);
                            vkCmdEndRenderPass(cmd);

                            VkUtil::transition_image(cmd, offscreen.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                            VkImageCopy copyRegion = {};

                            copyRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                            copyRegion.srcSubresource.baseArrayLayer = 0;
                            copyRegion.srcSubresource.mipLevel       = 0;
                            copyRegion.srcSubresource.layerCount     = 1;
                            copyRegion.srcOffset                     = {0, 0, 0};

                            copyRegion.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                            copyRegion.dstSubresource.baseArrayLayer = f;
                            copyRegion.dstSubresource.mipLevel       = m;
                            copyRegion.dstSubresource.layerCount     = 1;
                            copyRegion.dstOffset                     = {0, 0, 0};

                            copyRegion.extent.width  = static_cast<uint32_t>(viewport.width);
                            copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
                            copyRegion.extent.depth  = 1;

                            vkCmdCopyImage(cmd,
                                           offscreen.Image,
                                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                           newTexture->Image,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           1,
                                           &copyRegion);


                            VkUtil::transition_image(cmd, offscreen.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
                        }
                    }

                    VkUtil::transition_image(cmd, newTexture->Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, numMips, 6);
                });

        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyFramebuffer(device, offscreen.Framebuffer, nullptr);
        vkFreeMemory(device, offscreen.Memory, nullptr);
        vkDestroyImageView(device, offscreen.View, nullptr);
        vkDestroyImage(device, offscreen.Image, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descirptorSetLayout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        return newTexture;
    }
    Texture* GenerateBRDFLUT()
    {
        const VkDevice       device = LogicalDevice::GetDevice();
        VkFormat             format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags    usage  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        constexpr uint32_t   dim    = 512;
        constexpr VkExtent3D extent{.width = dim, .height = dim, .depth = 1};

        Texture* newTexture = new Texture();
        newTexture->ImageExtent                      = extent;
        newTexture->ImageFormat                      = format;
        VkImageCreateInfo imageCreateInfo           = vkinit::image_create_info(format, usage, newTexture->ImageExtent);
        imageCreateInfo.mipLevels                   = 1;
        imageCreateInfo.arrayLayers                 = 1;
        imageCreateInfo.samples                     = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling                      = VK_IMAGE_TILING_OPTIMAL;
        constexpr VmaAllocationCreateInfo allocInfo = {.usage         = VMA_MEMORY_USAGE_GPU_ONLY,
                                                       .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imageCreateInfo, &allocInfo, &newTexture->Image, &newTexture->Allocation, &newTexture->Info));


        // ImageView

        VkImageViewCreateInfo imageViewInfo       = vkinit::imageviewCreateInfo(format, newTexture->Image, VK_IMAGE_ASPECT_COLOR_BIT);
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.layerCount = 1;
        VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &newTexture->ImageView));

        // Sampler
        VkSamplerCreateInfo samplerInfo = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        samplerInfo.magFilter           = VK_FILTER_LINEAR;
        samplerInfo.minFilter           = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode          = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW        = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.minLod              = 0.0f;
        samplerInfo.maxLod              = 1.0f;
        samplerInfo.borderColor         = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &newTexture->Sampler));


        VkAttachmentDescription attDesc = {};
        // Color attachment
        attDesc.format                       = format;
        attDesc.samples                      = VK_SAMPLE_COUNT_1_BIT;
        attDesc.loadOp                       = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attDesc.storeOp                      = VK_ATTACHMENT_STORE_OP_STORE;
        attDesc.stencilLoadOp                = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attDesc.stencilStoreOp               = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attDesc.initialLayout                = VK_IMAGE_LAYOUT_UNDEFINED;
        attDesc.finalLayout                  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        VkAttachmentReference colorReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

        VkSubpassDescription subpassDescription = {};
        subpassDescription.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount = 1;
        subpassDescription.pColorAttachments    = &colorReference;

        // Use subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;
        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPass renderPass;

        {
            VkRenderPassCreateInfo renderPassInfo = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO, .pNext = nullptr};
            renderPassInfo.attachmentCount        = 1;
            renderPassInfo.pAttachments           = &attDesc;
            renderPassInfo.subpassCount           = 1;
            renderPassInfo.pSubpasses             = &subpassDescription;
            renderPassInfo.dependencyCount        = 2;
            renderPassInfo.pDependencies          = dependencies.data();

            VK_CHECK(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass));
        }
        VkFramebufferCreateInfo frameBufferInfo = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, .pNext = nullptr};
        frameBufferInfo.renderPass              = renderPass;
        frameBufferInfo.attachmentCount         = 1;
        frameBufferInfo.pAttachments            = &newTexture->ImageView;
        frameBufferInfo.width                   = dim;
        frameBufferInfo.height                  = dim;
        frameBufferInfo.layers                  = 1;

        VkFramebuffer frameBuffer;
        VK_CHECK(vkCreateFramebuffer(device, &frameBufferInfo, nullptr, &frameBuffer));

        VkDescriptorSetLayout                     descriptorLayout;
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings    = {};
        VkDescriptorSetLayoutCreateInfo           descriptorLayoutInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        descriptorLayoutInfo.pBindings                                 = setLayoutBindings.data();
        descriptorLayoutInfo.bindingCount                              = static_cast<uint32_t>(setLayoutBindings.size());
        VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorLayoutInfo, nullptr, &descriptorLayout));

        constexpr std::array<VkDescriptorPoolSize, 1> poolSizes = {VkDescriptorPoolSize{
                .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
        }};

        VkDescriptorPoolCreateInfo descriptorPoolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr};
        descriptorPoolInfo.pPoolSizes                 = poolSizes.data();
        descriptorPoolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.maxSets                    = 2;

        VkDescriptorPool descriptorPool;
        VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

        VkDescriptorSet             descritporSet;
        VkDescriptorSetAllocateInfo allocinfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr};
        allocinfo.descriptorPool              = descriptorPool;
        allocinfo.pSetLayouts                 = &descriptorLayout;
        allocinfo.descriptorSetCount          = 1;
        VK_CHECK(vkAllocateDescriptorSets(device, &allocinfo, &descritporSet));


        VkPipelineLayout           pipelineLayout;
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .pNext = nullptr};
        pipelineLayoutInfo.pSetLayouts                = &descriptorLayout;
        pipelineLayoutInfo.setLayoutCount             = 1;
        VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));

        const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetVertexInput(vertexInputInfo);

        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.EnableDepthClamp(VK_FALSE);
        pipelineBuilder.m_Multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipelineBuilder.m_Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        pipelineBuilder.m_Multisampling.flags                = 0;
        pipelineBuilder.SetLayout(pipelineLayout);
        // pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT);
        // pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_SCISSOR);
        pipelineBuilder.m_Rasterizer.lineWidth = 1.0f;

        const std::string brdflutPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/genbrdflut.frag.spv";

        VkShaderModule brdflutFragShader;
        if (!VkPipelines::LoadShaderModule(brdflutPath.c_str(), device, &brdflutFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string brdflutVertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/genbrdflut.vert.spv";
        VkShaderModule    brdflutVertexShader;
        if (!VkPipelines::LoadShaderModule(brdflutVertexShaderPath.c_str(), device, &brdflutVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }
        pipelineBuilder.SetShaders(brdflutVertexShader, brdflutFragShader);

        VkPipeline pipeline = pipelineBuilder.BuildPipline(device, 1, renderPass);

        vkDestroyShaderModule(device, brdflutFragShader, nullptr);
        vkDestroyShaderModule(device, brdflutVertexShader, nullptr);

        VkClearValue clearValues[1];
        clearValues[0].color = {{0.f, 0.f, 0.f, 1.0f}};

        VkRenderPassBeginInfo renderPassInfo    = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO, .pNext = nullptr};
        renderPassInfo.renderPass               = renderPass;
        renderPassInfo.renderArea.extent.width  = dim;
        renderPassInfo.renderArea.extent.height = dim;
        renderPassInfo.clearValueCount          = 1;
        renderPassInfo.pClearValues             = clearValues;
        renderPassInfo.framebuffer              = frameBuffer;

        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    constexpr VkViewport viewport = {.width = (float)dim, .height = (float)dim, .minDepth = 0, .maxDepth = 1.0f};
                    VkRect2D             scissor{};
                    scissor.extent.width  = dim;
                    scissor.extent.height = dim;
                    scissor.offset.x      = 0;
                    scissor.offset.y      = 0;
                    vkCmdSetViewport(cmd, 0, 1, &viewport);
                    vkCmdSetScissor(cmd, 0, 1, &scissor);
                    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
                    vkCmdDraw(cmd, 3, 1, 0, 0);
                    vkCmdEndRenderPass(cmd);
                });

        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        vkDestroyFramebuffer(device, frameBuffer, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        return newTexture;
    }

} // namespace MamontEngine
