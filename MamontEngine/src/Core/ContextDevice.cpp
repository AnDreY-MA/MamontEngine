#include "ContextDevice.h"

#include <VkBootstrap.h>
#include "Window.h"
#include "VkInitializers.h"
#include "VkImages.h"
#include "Graphics/Vulkan/GPUBuffer.h"

#include <vk_mem_alloc.h>


namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;

    VkContextDevice::~VkContextDevice()
    {
        for (auto &frame : m_Frames)
        {
            frame.Deleteions.Flush();
        }

        vkDestroySurfaceKHR(Instance, Surface, nullptr);

        vmaDestroyAllocator(Allocator);

        vkDestroyDevice(Device, nullptr);
        vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
        vkDestroyInstance(Instance, nullptr);
    }

    void VkContextDevice::Init(WindowCore *inWindow)
    {
        vkb::InstanceBuilder builder;

        const auto instRet = builder.set_app_name("Mamont App")
                                     .request_validation_layers(bUseValidationLayers)
                                     .use_default_debug_messenger()
                                     .require_api_version(1, 3, 290)
                                     .build();

        const vkb::Instance vkbInstance = instRet.value();
        Instance        = vkbInstance.instance;
        DebugMessenger  = vkbInstance.debug_messenger;

        if (!Instance || !DebugMessenger)
        {
            fmt::println("m_ContextDevice.Instance || m_ContextDevice.DebugMessenger is NULL");
        }

        inWindow->CreateSurface(Instance, &Surface);

        const auto deviceFeatures = VkPhysicalDeviceFeatures{
                .imageCubeArray    = VK_TRUE,
                .geometryShader    = VK_TRUE,
                .depthClamp        = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
        };

        VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features.dynamicRendering = VK_TRUE;
        features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features12.bufferDeviceAddress               = VK_TRUE;
        features12.descriptorIndexing                = VK_TRUE;
        features12.shaderInt8                        = VK_TRUE;
        features12.uniformAndStorageBuffer8BitAccess = VK_TRUE;
        features12.runtimeDescriptorArray            = VK_TRUE;


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        const vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                                           .set_required_features(deviceFeatures)
                                                           .set_required_features_13(features)
                                                           .set_required_features_12(features12)
                                                           .set_surface(Surface)
                                                           .select()
                                                           .value();

        const vkb::DeviceBuilder deviceBuilder{physicalDevice};

        const vkb::Device vkbDevice = deviceBuilder.build().value();

        Device    = vkbDevice.device;
        ChosenGPU = physicalDevice.physical_device;
        fmt::print("Name {}", physicalDevice.name);

        GraphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = ChosenGPU;
        allocatorInfo.device                 = Device;
        allocatorInfo.instance               = Instance;
        allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &Allocator);

    }

    AllocatedBuffer VkContextDevice::CreateBuffer(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage) const
    {
        VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.pNext              = nullptr;
        bufferInfo.size               = inAllocSize;
        bufferInfo.usage              = inUsage;

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage                   = inMemoryUsage;
        vmaAllocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer newBuffer{};

        VK_CHECK(vmaCreateBuffer(Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, &newBuffer.Info));

        return newBuffer;
    }

    GPUMeshBuffers VkContextDevice::CreateGPUMeshBuffer(std::span<uint32_t> inIndices, std::span<Vertex> inVertices)
    {
        const size_t vertexBufferSize{inVertices.size() * sizeof(Vertex)};
        const size_t indexBufferSize{inIndices.size() * sizeof(uint32_t)};

        GPUMeshBuffers newSurface{};

        newSurface.VertexBuffer =
                CreateBuffer(vertexBufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);

        VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.VertexBuffer.Buffer};
        newSurface.VertexBufferAddress              = vkGetBufferDeviceAddress(Device, &deviceAddressInfo);

        newSurface.IndexBuffer = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(Allocator, staging.Allocation, &allocInfo);
        void *data = allocInfo.pMappedData;
        //void *data = staging.Allocation->GetMappedData;   

        memcpy(data, inVertices.data(), vertexBufferSize);
        memcpy((char *)data + vertexBufferSize, inIndices.data(), indexBufferSize);

        ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkBufferCopy vertexCopy{0};
                    vertexCopy.dstOffset = 0;
                    vertexCopy.srcOffset = 0;
                    vertexCopy.size      = vertexBufferSize;

                    vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.VertexBuffer.Buffer, 1, &vertexCopy);

                    VkBufferCopy indexCopy{0};
                    indexCopy.dstOffset = 0;
                    indexCopy.srcOffset = vertexBufferSize;
                    indexCopy.size      = indexBufferSize;

                    vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.IndexBuffer.Buffer, 1, &indexCopy);
                });

        DestroyBuffer(staging);

        return newSurface;
    }
    
    void VkContextDevice::DestroyBuffer(const AllocatedBuffer &inBuffer) const
    {
        vmaDestroyBuffer(Allocator, inBuffer.Buffer, inBuffer.Allocation);

    }

    AllocatedImage VkContextDevice::CreateImage(const VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const
    {
        AllocatedImage newImage;
        newImage.ImageFormat = inFormat;
        newImage.ImageExtent = inSize;

        VkImageCreateInfo img_info = vkinit::image_create_info(inFormat, inUsage, inSize);
        if (inIsMipMapped)
        {
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(inSize.width, inSize.height)))) + 1;
        }

        // always allocate images on dedicated GPU memory
        VmaAllocationCreateInfo allocinfo = {};
        allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
        allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(Allocator, &img_info, &allocinfo, &newImage.Image, &newImage.Allocation, nullptr));


        VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        if (inFormat == VK_FORMAT_D32_SFLOAT)
        {
            aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo view_info       = vkinit::imageview_create_info(inFormat, newImage.Image, aspectFlag);
        view_info.subresourceRange.levelCount = img_info.mipLevels;

        VK_CHECK(vkCreateImageView(Device, &view_info, nullptr, &newImage.ImageView));

        return newImage;
    }

    AllocatedImage VkContextDevice::CreateImage(void *inData, VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped)
    {
        size_t          data_size    = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer = CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(uploadbuffer.Info.pMappedData, inData, data_size);

        AllocatedImage new_image = CreateImage(inSize, inFormat, inUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inIsMipMapped);

        ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkUtil::transition_image(cmd, new_image.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset      = 0;
                    copyRegion.bufferRowLength   = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel       = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount     = 1;
                    copyRegion.imageExtent                     = inSize;

                    vkCmdCopyBufferToImage(cmd, uploadbuffer.Buffer, new_image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                    if (inIsMipMapped)
                    {
                        VkUtil::generate_mipmaps(cmd, new_image.Image, VkExtent2D{new_image.ImageExtent.width, new_image.ImageExtent.height});
                    }
                    else
                    {
                        VkUtil::transition_image(cmd, new_image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    }
                });

        DestroyBuffer(uploadbuffer);

        return new_image;
    }

    void VkContextDevice::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const
    {
        VK_CHECK(vkResetFences(Device, 1, &ImmFence));
        VK_CHECK(vkResetCommandBuffer(ImmCommandBuffer, 0));

        VkCommandBuffer cmd = ImmCommandBuffer;

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        inFunction(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(GraphicsQueue, 1, &submit, ImmFence));

        VK_CHECK(vkWaitForFences(Device, 1, &ImmFence, true, 9999999999));
    }

    void VkContextDevice::DestroyImage(const AllocatedImage &inImage)
    {
        vkDestroyImageView(Device, inImage.ImageView, nullptr);
        vmaDestroyImage(Allocator, inImage.Image, inImage.Allocation);
    }

    FrameData &VkContextDevice::GetCurrentFrame()
    {
        return m_Frames.at(m_FrameNumber % FRAME_OVERLAP);
    }
}