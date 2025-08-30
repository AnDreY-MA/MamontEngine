#include "ContextDevice.h"

#include <VkBootstrap.h>
#include "Window.h"
#include "VkInitializers.h"
#include "VkImages.h"
#include "Graphics/Vulkan/MeshBuffer.h"
#include <glm/gtx/transform.hpp>

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

        constexpr auto deviceFeatures = VkPhysicalDeviceFeatures{
                .imageCubeArray    = VK_TRUE,
                .geometryShader    = VK_TRUE,
                .depthClamp        = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
        };

        constexpr VkPhysicalDeviceVulkan14Features features14 = {
            .maintenance5 = VK_TRUE,
            .pushDescriptor = VK_TRUE
        };

        constexpr VkPhysicalDeviceVulkan13Features features = {
            .synchronization2 = VK_TRUE,
            .dynamicRendering = VK_TRUE
        };

        constexpr VkPhysicalDeviceVulkan12Features features12 = {
            .uniformAndStorageBuffer8BitAccess = VK_TRUE,
            .shaderInt8 = VK_TRUE,
            .descriptorIndexing = VK_TRUE,
            .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
            .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
            .descriptorBindingUpdateUnusedWhilePending    = VK_TRUE,
            .descriptorBindingPartiallyBound              = VK_TRUE,
            .descriptorBindingVariableDescriptorCount     = VK_TRUE,
            .runtimeDescriptorArray                       = VK_TRUE,
            .timelineSemaphore                            = VK_TRUE,
            .bufferDeviceAddress = VK_TRUE
        };


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        const vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 4)
                                                           .set_required_features(deviceFeatures)
                                                           .set_required_features_14(features14)
                                                           .set_required_features_13(features)
                                                           .set_required_features_12(features12)
                                                           .set_surface(Surface)
                                                           .select()
                                                           .value();

        const vkb::DeviceBuilder deviceBuilder{physicalDevice};

        const vkb::Device vkbDevice = deviceBuilder.build().value();

        Device    = vkbDevice.device;
        m_PhysicalDevice = std::make_unique<PhysicalDevice>(physicalDevice.physical_device);
        fmt::print("Name {}", physicalDevice.name);

        GraphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = m_PhysicalDevice->GetDevice();
        allocatorInfo.device                 = Device;
        allocatorInfo.instance               = Instance;
        allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &Allocator);

    }

    AllocatedBuffer VkContextDevice::CreateBuffer(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage) const
    {
        const VkBufferCreateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext              = nullptr,
            .size               = inAllocSize,
            .usage = inUsage
        };

        const VmaAllocationCreateInfo vmaAllocInfo = {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage                   = inMemoryUsage,
        };
        
        AllocatedBuffer newBuffer{};
        VK_CHECK(vmaCreateBuffer(Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, &newBuffer.Info));

        return newBuffer;
    }

    MeshBuffer VkContextDevice::CreateGPUMeshBuffer(std::span<uint32_t> inIndices, std::span<Vertex> inVertices) const
    {
        const size_t vertexBufferSize{inVertices.size() * sizeof(Vertex)};
        const size_t indexBufferSize{inIndices.size() * sizeof(uint32_t)};

        MeshBuffer newSurface{};

        newSurface.VertexBuffer =
                CreateBuffer(vertexBufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);

        const VkBufferDeviceAddressInfo deviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
            .buffer = newSurface.VertexBuffer.Buffer
        };
        newSurface.VertexBufferAddress              = vkGetBufferDeviceAddress(Device, &deviceAddressInfo);

        newSurface.IndexBuffer = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        const AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(Allocator, staging.Allocation, &allocInfo);
        void *data = allocInfo.pMappedData;
        //void *data = staging.Allocation->GetMappedData;   

        memcpy(data, inVertices.data(), vertexBufferSize);
        memcpy((char *)data + vertexBufferSize, inIndices.data(), indexBufferSize);

        ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    const VkBufferCopy vertexCopy = {
                        .srcOffset = 0, 
                        .dstOffset = 0, 
                        .size = vertexBufferSize
                    };

                    vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.VertexBuffer.Buffer, 1, &vertexCopy);

                    const VkBufferCopy indexCopy = {
                        .srcOffset = vertexBufferSize, 
                        .dstOffset = 0, 
                        .size = indexBufferSize
                    };

                    vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.IndexBuffer.Buffer, 1, &indexCopy);
                });

        DestroyBuffer(staging);

        return newSurface;
    }
    
    void VkContextDevice::DestroyBuffer(const AllocatedBuffer &inBuffer) const
    {
        vmaDestroyBuffer(Allocator, inBuffer.Buffer, inBuffer.Allocation);

    }

    AllocatedImage VkContextDevice::CreateImage(const VkExtent3D& inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const
    {
        AllocatedImage newImage {};
        newImage.ImageFormat = inFormat;
        newImage.ImageExtent = inSize;

        VkImageCreateInfo img_info = vkinit::image_create_info(inFormat, inUsage, inSize);
        if (inIsMipMapped)
        {
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(inSize.width, inSize.height)))) + 1;
        }

        // always allocate images on dedicated GPU memory
        constexpr auto allocinfo = VmaAllocationCreateInfo {
            .usage                   = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_CHECK(vmaCreateImage(Allocator, &img_info, &allocinfo, &newImage.Image, &newImage.Allocation, nullptr));

        const VkImageAspectFlags aspectFlag = inFormat == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT  : VK_IMAGE_ASPECT_COLOR_BIT;

        const VkImageViewCreateInfo view_info       = vkinit::imageview_create_info(inFormat, newImage.Image, aspectFlag, img_info.mipLevels);

        VK_CHECK(vkCreateImageView(Device, &view_info, nullptr, &newImage.ImageView));

        return newImage;
    }

    AllocatedImage VkContextDevice::CreateImage(void *inData, const VkExtent3D& inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const
    {
        const size_t          data_size    = inSize.depth * inSize.width * inSize.height * 4;
        const AllocatedBuffer uploadbuffer = CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

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

    void VkContextDevice::InitDefaultImages()
    {
        const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        WhiteImage =
                CreateImage((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);

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
        ErrorCheckerboardImage =
                CreateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, true);
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

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        const VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(GraphicsQueue, 1, &submit, ImmFence));

        VK_CHECK(vkWaitForFences(Device, 1, &ImmFence, true, 9999999999));
    }

    void VkContextDevice::DestroyImage(const AllocatedImage &inImage) const
    {
        vkDestroyImageView(Device, inImage.ImageView, nullptr);
        vmaDestroyImage(Allocator, inImage.Image, inImage.Allocation);
    }

    void VkContextDevice::InitCommands(DeletionQueue &inDeletionQueue)
    {
        const VkCommandPoolCreateInfo commandPoolInfo =
                vkinit::command_pool_create_info(GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(Device, &commandPoolInfo, nullptr, &GetFrameAt(i).CommandPool));

            const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(GetFrameAt(i).CommandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(Device, &cmdAllocInfo, &GetFrameAt(i).MainCommandBuffer));

            inDeletionQueue.PushFunction([=]() { 
                vkDestroyCommandPool(Device, GetFrameAt(i).CommandPool, nullptr); });
        }

        VK_CHECK(vkCreateCommandPool(Device, &commandPoolInfo, nullptr, &ImmCommandPool));

        const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(ImmCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(Device, &cmdAllocInfo, &ImmCommandBuffer));

        inDeletionQueue.PushFunction([=]() { 
            vkDestroyCommandPool(Device, ImmCommandPool, nullptr); });
         
    }

    void VkContextDevice::InitSyncStructeres()
    {
        const VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

        VK_CHECK(vkCreateFence(Device, &fenceCreateInfo, nullptr, &ImmFence));

        const VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);

        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(Device, &fenceCreateInfo, nullptr, &GetFrameAt(i).RenderFence));

            VK_CHECK(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &GetFrameAt(i).SwapchainSemaphore));

            VK_CHECK(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &GetFrameAt(i).RenderSemaphore));
        }
    }

    void VkContextDevice::DestroySyncStructeres()
    {
        vkDestroyFence(Device, ImmFence, nullptr);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            vkDestroyFence(Device, GetFrameAt(i).RenderFence, nullptr);
            vkDestroySemaphore(Device, GetFrameAt(i).SwapchainSemaphore, nullptr);
            vkDestroySemaphore(Device, GetFrameAt(i).RenderSemaphore, nullptr);
        }
    }

    void VkContextDevice::InitDescriptors()
    {
        std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}};

        GlobalDescriptorAllocator.init_pool(Device, 10, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            DrawImageDescriptorLayout = builder.Build(Device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            GPUSceneDataDescriptorLayout = builder.Build(Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }


        DrawImageDescriptors = GlobalDescriptorAllocator.Allocate(Device, DrawImageDescriptorLayout);

        {
            DescriptorWriter writer;
            writer.WriteImage(0, Image.DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            writer.UpdateSet(Device, DrawImageDescriptors);
        }

        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                                                                               {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                                                                               {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                                                                               {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};

        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            GetFrameAt(i).FrameDescriptors = DescriptorAllocatorGrowable{};
            GetFrameAt(i).FrameDescriptors.Init(Device, 1000, frame_sizes);
        }
    }

    void VkContextDevice::DestroyDescriptors()
    {
        vkDestroyDescriptorSetLayout(Device, DrawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(Device, GPUSceneDataDescriptorLayout, nullptr);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).FrameDescriptors.DestroyPools(Device);
        }
    }

    void VkContextDevice::InitSamples()
    {
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(Device, &sampl, nullptr, &DefaultSamplerNearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;

        vkCreateSampler(Device, &sampl, nullptr, &DefaultSamplerLinear);
    }

    void VkContextDevice::CreateAllFrameOffscreans(const VkExtent2D &inExtent, const VkFormat inFormat, std::vector<VkImageView> inImageView)
    {
        for (size_t i = 0; i < 3; i++)
        {
            FrameData &frame = GetFrameAt(i);
            CreateFrameOffscreen(frame, inExtent, inFormat, Swapchain.GetImageView(i));
        }
    }
    
    void VkContextDevice::CreateFrameOffscreen(FrameData &inFrame, const VkExtent2D &inExtent, const VkFormat inFormat, VkImageView inImageView)
    {
        /*inFrame.Deleteions.PushFunction(
                [=, this]()
                {
                    ImGui_ImplVulkan_RemoveTexture(inFrame.OffscreenImage.ImTextID);
                    DestroyImage(inFrame.OffscreenImage);
                });*/

        VkExtent3D size3d = {inExtent.width, inExtent.height, 1};

        const VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        inFrame.OffscreenImage = CreateImage(size3d, Swapchain.GetImageFormat(), usage, true);
        inFrame.Deleteions.PushFunction([=]() { 
            DestroyImage(inFrame.OffscreenImage);
                    });


        //inFrame.OffscreenImage.ImTextID = ImGui_ImplVulkan_AddTexture(DefaultSamplerLinear, inImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        //Image.DrawImage.ImTextID = ImGui_ImplVulkan_AddTexture(DefaultSamplerLinear, Image.DrawImage.ImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void VkContextDevice::InitSwapchain(const VkExtent2D &inWindowExtent)
    {
        Swapchain.Init(*this, inWindowExtent, Image);
    }

    void VkContextDevice::ResizeSwapchain(const VkExtent2D& inWindowExtent)
    {
        vkDeviceWaitIdle(Device);

        Swapchain.Destroy(Device);

        Swapchain.Create(*this, inWindowExtent);

        CreateAllFrameOffscreans(
                Swapchain.GetExtent(), Swapchain.GetImageFormat(), Swapchain.GetImageViews());
    }

    void VkContextDevice::DestroyImage()
    {
        vkDestroyImageView(Device, Image.DrawImage.ImageView, nullptr);
        vmaDestroyImage(Allocator, Image.DrawImage.Image, Image.DrawImage.Allocation);

        vkDestroyImageView(Device, Image.DepthImage.ImageView, nullptr);
        vmaDestroyImage(Allocator, Image.DepthImage.Image, Image.DepthImage.Allocation);
    }

    FrameData &VkContextDevice::GetCurrentFrame()
    {
        return m_Frames.at(m_FrameNumber % FRAME_OVERLAP);
    }

    VkPhysicalDevice VkContextDevice::GetPhysicalDevice() const
    {
        return m_PhysicalDevice->GetDevice();
    }
}