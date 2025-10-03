#include "ContextDevice.h"

#include "Window.h"
#include "VkInitializers.h"
#include "VkImages.h"
#include "Graphics/Vulkan/MeshBuffer.h"
#include <glm/gtx/transform.hpp>
#include "ECS/SceneRenderer.h"
#include "tracy/TracyVulkan.hpp"
#include "Graphics/Vulkan/Allocator.h"

#include <vk_mem_alloc.h>

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;

    VkContextDevice::VkContextDevice(WindowCore *inWindow)
    {
        vkb::InstanceBuilder builder;

        const auto instRet = builder.set_app_name("Mamont App")
                                     .request_validation_layers(bUseValidationLayers)
                                     .use_default_debug_messenger()
                                     .require_api_version(1, 4, 321)
                                     .build();

        const vkb::Instance vkbInstance = instRet.value();
        Instance                        = vkbInstance.instance;
        DebugMessenger                  = vkbInstance.debug_messenger;

        if (!Instance || !DebugMessenger)
        {
            fmt::println("m_ContextDevice.Instance || m_ContextDevice.DebugMessenger is NULL");
        }

        inWindow->CreateSurface(Instance, &Surface);

        constexpr auto deviceFeatures = VkPhysicalDeviceFeatures{
                .imageCubeArray    = VK_TRUE,
                .geometryShader    = VK_TRUE,
                .multiDrawIndirect = VK_TRUE,
                .drawIndirectFirstInstance = VK_TRUE,
                .depthClamp        = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
        };

        constexpr VkPhysicalDeviceVulkan14Features features14 = {.maintenance5 = VK_TRUE, .pushDescriptor = VK_TRUE};

        constexpr VkPhysicalDeviceVulkan13Features features = {.synchronization2 = VK_TRUE, .dynamicRendering = VK_TRUE};

        constexpr VkPhysicalDeviceVulkan12Features features12 = {.uniformAndStorageBuffer8BitAccess            = VK_TRUE,
                                                                 .shaderInt8                                   = VK_TRUE,
                                                                 .descriptorIndexing                           = VK_TRUE,
                                                                 .shaderSampledImageArrayNonUniformIndexing    = VK_TRUE,
                                                                 .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
                                                                 .descriptorBindingUpdateUnusedWhilePending    = VK_TRUE,
                                                                 .descriptorBindingPartiallyBound              = VK_TRUE,
                                                                 .descriptorBindingVariableDescriptorCount     = VK_TRUE,
                                                                 .runtimeDescriptorArray                       = VK_TRUE,
#ifdef PROFILING
                                                                 .hostQueryReset                               = VK_TRUE,
#endif
                                                                 .timelineSemaphore                            = VK_TRUE,
                                                                 .bufferDeviceAddress                          = VK_TRUE};


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        std::vector<const char *> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME};

#ifdef PROFILING
        extensions.push_back("VK_EXT_calibrated_timestamps");
        extensions.push_back("VK_EXT_host_query_reset");
#endif
        const vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 4)
                                                           .set_required_features(deviceFeatures)
                                                           .set_required_features_14(features14)
                                                           .set_required_features_13(features)
                                                           .set_required_features_12(features12)
                                                           .set_surface(Surface)
                                                           .add_required_extensions(extensions) 
                                                           .select()
                                                           .value();
        fmt::print("Name {}", physicalDevice.name);

        const vkb::DeviceBuilder deviceBuilder{physicalDevice};

        const vkb::Device vkbDevice = deviceBuilder.build().value();

        Device           = vkbDevice.device;
        m_PhysicalDevice = std::make_unique<PhysicalDevice>(physicalDevice.physical_device);

        m_GraphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        const VmaAllocatorCreateInfo allocatorInfo = {.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                                      .physicalDevice = m_PhysicalDevice->GetDevice(),
                                                      .device         = Device,
                                                      .instance       = Instance};
        Allocator::Init(allocatorInfo);
        //InitShadowImages();

    }

    VkContextDevice::~VkContextDevice()
    {
        for (auto &frame : m_Frames)
        {
            frame.Deleteions.Flush();
        }

        DestroyImage(CascadeDepthImage.Image);
        vkDestroySampler(Device, CascadeDepthImage.Sampler, nullptr);
        vkDestroySurfaceKHR(Instance, Surface, nullptr);

        Allocator::Destroy();

        vkDestroyDevice(Device, nullptr);
        vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
        vkDestroyInstance(Instance, nullptr);
    }

    void VkContextDevice::DestroyFrameData()
    {
        DestroyImages();
        DestroyCommands();
        DestroySyncStructeres();
        DestroyDescriptors();
        DestroySceneBuffers(); 

    #ifdef PROFILING
        for (size_t i{ 0 }; i < FRAME_OVERLAP; ++i)
        {
            tracy::DestroyVkContext(GetFrameAt(i).TracyContext);
        }
    #endif
    
    }

    MeshBuffer VkContextDevice::CreateGPUMeshBuffer(std::span<uint32_t> inIndices, std::span<Vertex> inVertices) const
    {
        const size_t vertexBufferSize{inVertices.size() * sizeof(Vertex)};
        const size_t indexBufferSize{inIndices.size() * sizeof(uint32_t)};

        MeshBuffer newSurface{};

        newSurface.VertexBuffer.Create(vertexBufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);

        const VkBufferDeviceAddressInfo deviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
            .buffer = newSurface.VertexBuffer.Buffer
        };
        newSurface.VertexBufferAddress              = vkGetBufferDeviceAddress(Device, &deviceAddressInfo);

        newSurface.IndexBuffer.Create(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        AllocatedBuffer staging;
        staging.Create(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(Allocator::GetAllocator(), staging.Allocation, &allocInfo);
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

        staging.Destroy();

        return newSurface;
    }

    AllocatedImage
    VkContextDevice::CreateImage(const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped, uint32_t arrayLayers) const
    {
        AllocatedImage newImage {};
        newImage.ImageFormat = inFormat;
        newImage.ImageExtent = inSize;

        VkImageCreateInfo img_info = vkinit::image_create_info(inFormat, inUsage, inSize, arrayLayers);
        if (inIsMipMapped)
        {
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(inSize.width, inSize.height)))) + 1;
        }

        // always allocate images on dedicated GPU memory
        constexpr auto allocinfo = VmaAllocationCreateInfo {
            .usage                   = VMA_MEMORY_USAGE_GPU_ONLY,
            .requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        };

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &img_info, &allocinfo, &newImage.Image, &newImage.Allocation, &newImage.Info));

        const VkImageAspectFlags aspectFlag = inFormat == VK_FORMAT_D32_SFLOAT ? VK_IMAGE_ASPECT_DEPTH_BIT  : VK_IMAGE_ASPECT_COLOR_BIT;

        const VkImageViewCreateInfo view_info       = vkinit::imageview_create_info(inFormat, newImage.Image, aspectFlag, img_info.mipLevels);

        VK_CHECK(vkCreateImageView(Device, &view_info, nullptr, &newImage.ImageView));

        return newImage;
    }

    AllocatedImage VkContextDevice::CreateImage(void *inData, const VkExtent3D& inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const
    {
        const size_t          data_size    = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer;
        uploadbuffer.Create(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

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

        uploadbuffer.Destroy();

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
        VK_CHECK(vkResetFences(Device, 1, &m_ImmFence));
        VK_CHECK(vkResetCommandBuffer(m_ImmCommandBuffer, 0));

        VkCommandBuffer cmd = m_ImmCommandBuffer;

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        inFunction(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        const VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_ImmFence));

        VK_CHECK(vkWaitForFences(Device, 1, &m_ImmFence, true, 9999999999));
    }

    void VkContextDevice::DestroyImage(const AllocatedImage &inImage) const
    {
        vkDestroyImageView(Device, inImage.ImageView, nullptr);
        vmaDestroyImage(Allocator::GetAllocator(), inImage.Image, inImage.Allocation);
    }

    void VkContextDevice::InitCommands()
    {
        const VkCommandPoolCreateInfo commandPoolInfo =
                vkinit::command_pool_create_info(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            VK_CHECK(vkCreateCommandPool(Device, &commandPoolInfo, nullptr, &GetFrameAt(i).CommandPool));

            const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(GetFrameAt(i).CommandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(Device, &cmdAllocInfo, &GetFrameAt(i).MainCommandBuffer));
        }

        VK_CHECK(vkCreateCommandPool(Device, &commandPoolInfo, nullptr, &m_ImmCommandPool));

        const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_ImmCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(Device, &cmdAllocInfo, &m_ImmCommandBuffer));

    #ifdef PROFILING

        m_TracyInfo.GetPhysicalDeviceCalibrateableTimeDomainsEXT =
                (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)vkGetInstanceProcAddr(Instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
        m_TracyInfo.GetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)vkGetInstanceProcAddr(Instance, "vkGetCalibratedTimestampsEXT");

        VK_CHECK(vkCreateCommandPool(Device, &commandPoolInfo, nullptr, &m_TracyInfo.CommandPool));

        const VkCommandBufferAllocateInfo tracyCmdAllocInfo = vkinit::command_buffer_allocate_info(m_TracyInfo.CommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(Device, &tracyCmdAllocInfo, &m_TracyInfo.CommandBuffer));
     #endif   
    }
    
    void VkContextDevice::DestroyCommands()
    {
        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(Device, GetFrameAt(i).CommandPool, nullptr);
        }
        vkDestroyCommandPool(Device, m_ImmCommandPool, nullptr);
        vkDestroyCommandPool(Device, m_TracyInfo.CommandPool, nullptr);

    }

    void VkContextDevice::InitSyncStructeres()
    {
        const VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

        VK_CHECK(vkCreateFence(Device, &fenceCreateInfo, nullptr, &m_ImmFence));

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
        vkDestroyFence(Device, m_ImmFence, nullptr);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            vkDestroyFence(Device, GetFrameAt(i).RenderFence, nullptr);
            vkDestroySemaphore(Device, GetFrameAt(i).SwapchainSemaphore, nullptr);
            vkDestroySemaphore(Device, GetFrameAt(i).RenderSemaphore, nullptr);
        }
    }

    void VkContextDevice::InitSceneBuffers()
    {
        for (size_t i{ 0 }; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).SceneDataBuffer.Create(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            GetFrameAt(i).FragmentBuffer.Create(sizeof(SceneDataFragment), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            GetFrameAt(i).CascadeViewProjMatrices.Create(sizeof(glm::mat4) * CASCADECOUNT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            
            //VK_CHECK(GetFrameAt(i).SceneDataBuffer.Map());
            //vkMapMemory(Device, GetFrameAt(i).SceneDataBuffer.Memory, 0, VK_WHOLE_SIZE, 0, &GetFrameAt(i).SceneDataBuffer.MappedData);
        }
    }
    
    void VkContextDevice::DestroySceneBuffers()
    {
        for (size_t i{ 0 }; i < FRAME_OVERLAP; i++)
        {
            GetFrameAt(i).SceneDataBuffer.Destroy();
            GetFrameAt(i).FragmentBuffer.Destroy();
            GetFrameAt(i).CascadeViewProjMatrices.Destroy();
            /*GetFrameAt(i).ShadowMatrixBuffer.Destroy();
            GetFrameAt(i).ShadowUBOBuffer.Destroy();*/
        }
    }

    void VkContextDevice::InitDescriptors()
    {
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
            DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3}, 
            DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}, 
            DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3}, 
            DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}
        };

        GlobalDescriptorAllocator.Init(Device, 1000, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            DrawImageDescriptorLayout = builder.Build(Device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            GPUSceneDataDescriptorLayout = builder.Build(Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }


        DrawImageDescriptors = GlobalDescriptorAllocator.Allocate(Device, DrawImageDescriptorLayout);

        {
            DescriptorWriter writer;
            writer.WriteImage(0, Image.DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            writer.UpdateSet(Device, DrawImageDescriptors);
        }


        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            /*GetFrameAt(i).FrameDescriptors = DescriptorAllocatorGrowable{};
            GetFrameAt(i).FrameDescriptors.Init(Device, 1000, frame_sizes);*/
            DescriptorWriter writer;
            writer.Clear();

            auto &globalDescriptor         = GetFrameAt(i).GlobalDescriptor;
            globalDescriptor       = GlobalDescriptorAllocator.Allocate(Device, GPUSceneDataDescriptorLayout);
            writer.WriteBuffer(0, GetFrameAt(i).SceneDataBuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteImage(1, CascadeDepthImage.Image.ImageView, CascadeDepthImage.Sampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteBuffer(2, GetFrameAt(i).FragmentBuffer.Buffer, sizeof(SceneDataFragment), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteBuffer(3, GetFrameAt(i).CascadeViewProjMatrices.Buffer, sizeof(glm::mat4) * CASCADECOUNT, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.UpdateSet(Device, globalDescriptor);

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

    void VkContextDevice::InitSwapchain(const VkExtent2D &inWindowExtent)
    {
        Swapchain.Init(*this, inWindowExtent, Image);
        
        if (Image.DrawImage.Image == VK_NULL_HANDLE)
        {
            fmt::println("NULL");
        }
        InitShadowImages();

    }

    void VkContextDevice::ResizeSwapchain(const VkExtent2D& inWindowExtent)
    {
        vkDeviceWaitIdle(Device);

        DestroyImages();

        Swapchain.Destroy(Device);

        Swapchain.Init(*this, inWindowExtent, Image);
    }

    void VkContextDevice::InitShadowImages()
    {
        const VkExtent3D shadowImageExtent{SHADOWMAP_DIMENSION, SHADOWMAP_DIMENSION, 1};
        auto imageInfo =
                vkinit::image_create_info(VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImageExtent);

        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = CASCADECOUNT;
        imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling      = VK_IMAGE_TILING_OPTIMAL;

        constexpr auto allocinfo = VmaAllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY, 
            .requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imageInfo, &allocinfo, &CascadeDepthImage.Image.Image, &CascadeDepthImage.Image.Allocation, &CascadeDepthImage.Image.Info));

        {
            auto imageViewInfo = vkinit::imageview_create_info(
                    VK_FORMAT_D32_SFLOAT, CascadeDepthImage.Image.Image, VK_IMAGE_ASPECT_DEPTH_BIT, CASCADECOUNT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
            //imageViewInfo.subresourceRange.layerCount = CASCADECOUNT;
            VK_CHECK(vkCreateImageView(Device, &imageViewInfo, nullptr, &CascadeDepthImage.Image.ImageView));
        }

        for (size_t i{ 0 }; i < CASCADECOUNT; i++)
        {
            auto imageViewInfo = vkinit::imageview_create_info(
                    VK_FORMAT_D32_SFLOAT, CascadeDepthImage.Image.Image, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY);
            imageViewInfo.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);

            VK_CHECK(vkCreateImageView(Device, &imageViewInfo, nullptr, &Cascades[i].View));
        }

        VkSamplerCreateInfo samplerInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .maxAnisotropy = 1.f};
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV  = samplerInfo.addressModeU;
        samplerInfo.addressModeW  = samplerInfo.addressModeU;
        samplerInfo.mipLodBias   = 0.0f;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.minLod        = 0.0f;
        samplerInfo.maxLod        = 1.0f;
        samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK(vkCreateSampler(Device, &samplerInfo, nullptr, &CascadeDepthImage.Sampler));
    }

    void VkContextDevice::InitTracyContext()
    {
        #ifdef PROFILING
        for (size_t i{ 0 }; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).TracyContext = tracy::CreateVkContext(m_PhysicalDevice->GetDevice(),
                                                                Device,
                                                                m_GraphicsQueue,
                                                                m_TracyInfo.CommandBuffer,
                                                                m_TracyInfo.GetPhysicalDeviceCalibrateableTimeDomainsEXT,
                                                                m_TracyInfo.GetCalibratedTimestampsEXT);

            char buf[50];
            const int  n = sprintf(buf, "Vulkan Context [%d]", i);
            TracyVkContextName(GetFrameAt(i).TracyContext, buf, n);
        }
        #endif
    }

    void VkContextDevice::DestroyImages() const
    {
        DestroyImage(Image.DrawImage);
        
        DestroyImage(Image.DepthImage);

        //DestroyImage(ShadowDepthImage);
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