#include "ContextDevice.h"

#include "Window.h"
#include "VkInitializers.h"
#include "VkImages.h"
#include <glm/gtx/transform.hpp>
#include "ECS/SceneRenderer.h"
#include "tracy/TracyVulkan.hpp"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Devices/LogicalDevice.h"

#include <vk_mem_alloc.h>

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = true;
    static VKAPI_ATTR VkBool32 VKAPI_CALL DetailedDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                                VkDebugUtilsMessageTypeFlagsEXT             type,
                                                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                                void                                       *pUserData)
    {

        // ✅ ВЫВОДИМ ВСЕ ДЕТАЛИ
        std::cerr << "\n=== VALIDATION LAYER ===" << std::endl;
        std::cerr << "Severity: ";
        switch (severity)
        {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                std::cerr << "VERBOSE";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                std::cerr << "INFO";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                std::cerr << "WARNING";
                break;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                std::cerr << "ERROR";
                break;
            default:
                std::cerr << "UNKNOWN";
        }
        std::cerr << " | Type: ";
        switch (type)
        {
            case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
                std::cerr << "GENERAL";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
                std::cerr << "VALIDATION";
                break;
            case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
                std::cerr << "PERFORMANCE";
                break;
            default:
                std::cerr << "UNKNOWN";
        }
        std::cerr << std::endl;

        std::cerr << "Message: " << pCallbackData->pMessage << std::endl;

        // ✅ ВЫВОДИМ ОБЪЕКТЫ
        if (pCallbackData->objectCount > 0)
        {
            std::cerr << "Objects involved (" << pCallbackData->objectCount << "):" << std::endl;
            for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
            {
                const auto &obj = pCallbackData->pObjects[i];
                std::cerr << "  [" << i << "] Type: " << obj.objectType << ", Handle: " << obj.objectHandle;
                if (obj.pObjectName)
                {
                    std::cerr << ", Name: '" << obj.pObjectName << "'";
                }
                std::cerr << std::endl;
            }
        }

        // ✅ ВЫВОДИМ QUEUE LABELS ЕСЛИ ЕСТЬ
        if (pCallbackData->queueLabelCount > 0)
        {
            std::cerr << "Queue labels:" << std::endl;
            for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
            {
                std::cerr << "  - " << pCallbackData->pQueueLabels[i].pLabelName << std::endl;
            }
        }

        // ✅ ВЫВОДИМ COMMAND BUFFER LABELS ЕСЛИ ЕСТЬ
        if (pCallbackData->cmdBufLabelCount > 0)
        {
            std::cerr << "Command buffer labels:" << std::endl;
            for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
            {
                std::cerr << "  - " << pCallbackData->pCmdBufLabels[i].pLabelName << std::endl;
            }
        }

        std::cerr << "=================================" << std::endl;

        return VK_FALSE;
    }

    VkContextDevice::VkContextDevice(WindowCore *inWindow)
    {
        vkb::InstanceBuilder builder;

        const auto instRet = builder.set_app_name("Mamont App")
                                     .request_validation_layers(bUseValidationLayers)
                                     //.use_default_debug_messenger()
                                     .require_api_version(1, 4, 321)
                                     .build();
        

        const vkb::Instance vkbInstance = instRet.value();
        Instance                        = vkbInstance.instance;
        if (bUseValidationLayers)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            debugCreateInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = DetailedDebugCallback;
            debugCreateInfo.pUserData       = nullptr;

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(Instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr)
            {
                func(Instance, &debugCreateInfo, nullptr, &DebugMessenger);
            }
            else
            {
                fmt::println("Failed to create debug messenger!");
            }
        }
        //DebugMessenger                  = vkbInstance.debug_messenger;
        if (vkbInstance.allocation_callbacks == nullptr)
        {
            fmt::println("allocation_callbacks == nullptr");
        }

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
                .depthBiasClamp    = VK_TRUE,
                .samplerAnisotropy = VK_TRUE,
                .shaderInt64 = VK_TRUE
        };

        constexpr VkPhysicalDeviceVulkan14Features features14 = {.maintenance6 = VK_TRUE, .pushDescriptor = VK_TRUE};

        constexpr VkPhysicalDeviceVulkan13Features features = {
            .synchronization2 = VK_TRUE, 
            .dynamicRendering = VK_TRUE
        };

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
                                                                 .bufferDeviceAddress                          = VK_TRUE,
        };


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

        LogicalDevice::InitDevice(vkbDevice.device);
        m_PhysicalDevice = std::make_unique<PhysicalDevice>(physicalDevice.physical_device);

        m_GraphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        const VmaAllocatorCreateInfo allocatorInfo = {.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                                      .physicalDevice = m_PhysicalDevice->GetDevice(),
                                                      .device         = LogicalDevice::GetDevice(),
                                                      .instance       = Instance};
        Allocator::Init(allocatorInfo);

        InitSwapchain(inWindow->GetExtent());

        InitCommands();

        InitSyncStructeres();

        InitSceneBuffers();

        InitDescriptors();

        InitTracyContext();

    }

    VkContextDevice::~VkContextDevice()
    {
        for (auto &frame : m_Frames)
        {
            frame.Deleteions.Flush();
        }

        DestroyImage(CascadeDepthImage.Image);
        vkDestroySampler(LogicalDevice::GetDevice(), CascadeDepthImage.Sampler, nullptr);
        vkDestroySurfaceKHR(Instance, Surface, nullptr);

        Allocator::Destroy();

        LogicalDevice::DestroyDevice();
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

    AllocatedImage
    VkContextDevice::CreateImage(const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped, uint32_t arrayLayers) const
    {
        AllocatedImage newImage {};
        newImage.ImageFormat = inFormat;
        newImage.ImageExtent = inSize;
        const VkDevice device = LogicalDevice::GetDevice();

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

        VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &newImage.ImageView));

        return newImage;
    }

    AllocatedImage VkContextDevice::CreateImage(void *inData, const VkExtent3D& inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const
    {
        const size_t          data_size    = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer = CreateStagingBuffer(data_size);
        //uploadbuffer.Create(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

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

        //std::cerr << "NewImage: " << new_image.Image << std::endl;

        return new_image;
    }

    void VkContextDevice::InitDefaultImages()
    {
        {
            const uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
            WhiteImage           = CreateImage((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
            std::cerr << "WhiteImage: " << ErrorCheckerboardImage.Image << std::endl;

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
            ErrorCheckerboardImage = CreateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, false);
            std::cerr << "ErrorCheckerboardImage: " << ErrorCheckerboardImage.Image << std::endl;
        }
        
    }

    void VkContextDevice::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const
    {
        const VkDevice device = LogicalDevice::GetDevice();
        VK_CHECK(vkResetFences(device, 1, &m_ImmFence));
        VK_CHECK(vkResetCommandBuffer(m_ImmCommandBuffer, 0));

        VkCommandBuffer cmd = m_ImmCommandBuffer;

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        inFunction(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        const VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_ImmFence));

        VK_CHECK(vkWaitForFences(device, 1, &m_ImmFence, true, 9999999999));
    }

    void VkContextDevice::DestroyImage(const AllocatedImage &inImage) const
    {
        vkDestroyImageView(LogicalDevice::GetDevice(), inImage.ImageView, nullptr);
        vmaDestroyImage(Allocator::GetAllocator(), inImage.Image, inImage.Allocation);
    }

    void VkContextDevice::InitCommands()
    {
        const VkCommandPoolCreateInfo commandPoolInfo =
                vkinit::command_pool_create_info(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        const VkDevice device = LogicalDevice::GetDevice();

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &GetFrameAt(i).CommandPool));

            const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(GetFrameAt(i).CommandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &GetFrameAt(i).MainCommandBuffer));
        }

        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &m_ImmCommandPool));

        const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_ImmCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &m_ImmCommandBuffer));

    #ifdef PROFILING

        m_TracyInfo.GetPhysicalDeviceCalibrateableTimeDomainsEXT =
                (PFN_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT)vkGetInstanceProcAddr(Instance, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT");
        m_TracyInfo.GetCalibratedTimestampsEXT = (PFN_vkGetCalibratedTimestampsEXT)vkGetInstanceProcAddr(Instance, "vkGetCalibratedTimestampsEXT");

        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &m_TracyInfo.CommandPool));

        const VkCommandBufferAllocateInfo tracyCmdAllocInfo = vkinit::command_buffer_allocate_info(m_TracyInfo.CommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &tracyCmdAllocInfo, &m_TracyInfo.CommandBuffer));
     #endif   
    }
    
    void VkContextDevice::DestroyCommands()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            vkDestroyCommandPool(device, GetFrameAt(i).CommandPool, nullptr);
        }
        vkDestroyCommandPool(device, m_ImmCommandPool, nullptr);
        vkDestroyCommandPool(device, m_TracyInfo.CommandPool, nullptr);

    }

    void VkContextDevice::InitSyncStructeres()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        const VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

        const VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &GetFrameAt(i).RenderFence));

            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &GetFrameAt(i).SwapchainSemaphore));
            std::cerr << "semaphore: " << GetFrameAt(i).SwapchainSemaphore << std::endl;

            //VK_CHECK(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &GetFrameAt(i).RenderSemaphore));
        }

        RenderCopleteSemaphores.resize(Swapchain.GetImages().size());
        for (auto& semaphore : RenderCopleteSemaphores)
        {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));

        }

        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &TransferFence));
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &m_ImmFence));

        {
            const VkExtent3D extent = {Swapchain.GetExtent().width, Swapchain.GetExtent().height, 1};

            PickingImages.resize(Swapchain.GetImages().size());
            for (size_t i{0}; i < Swapchain.GetImages().size(); ++i)
            {
                PickingImages[i] = CreateImage(
                        extent, VK_FORMAT_R32G32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

                ImmediateSubmit([&](VkCommandBuffer cmd)
                        { VkUtil::transition_image(cmd, PickingImages[i].Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL); });

                // std::cerr << "PickingImage: " << PickingImage.Image << std::endl;
            }
        }

    }

    void VkContextDevice::DestroySyncStructeres()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        vkDestroyFence(device, m_ImmFence, nullptr);
        vkDestroyFence(device, TransferFence, nullptr);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            vkDestroyFence(device, GetFrameAt(i).RenderFence, nullptr);
            vkDestroySemaphore(device, GetFrameAt(i).SwapchainSemaphore, nullptr);
            //vkDestroySemaphore(Device, GetFrameAt(i).RenderSemaphore, nullptr);
        }
        for (auto& semaphore : RenderCopleteSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

    }

    void VkContextDevice::InitSceneBuffers()
    {
        for (size_t i{ 0 }; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).SceneDataBuffer.Create(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            GetFrameAt(i).ShadowUBOBuffer.Create(sizeof(ShadowCascadeUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            GetFrameAt(i).ShadowmMatrixBuffer.Create(sizeof(ShadowCascadeMatrices), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            
            //VK_CHECK(GetFrameAt(i).SceneDataBuffer.Map());
            //vkMapMemory(Device, GetFrameAt(i).SceneDataBuffer.Memory, 0, VK_WHOLE_SIZE, 0, &GetFrameAt(i).SceneDataBuffer.MappedData);
        }
    }
    
    void VkContextDevice::DestroySceneBuffers()
    {
        for (size_t i{ 0 }; i < FRAME_OVERLAP; i++)
        {
            GetFrameAt(i).SceneDataBuffer.Destroy();
            GetFrameAt(i).ShadowUBOBuffer.Destroy();
            GetFrameAt(i).ShadowmMatrixBuffer.Destroy();
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
        const VkDevice device = LogicalDevice::GetDevice();

        GlobalDescriptorAllocator.Init(device, 10000, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            DrawImageDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        DrawImageDescriptors = GlobalDescriptorAllocator.Allocate(device, DrawImageDescriptorLayout);
        {
            DescriptorWriter writer;
            writer.WriteImage(0, Image.DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            writer.UpdateSet(device, DrawImageDescriptors);
        }
        std::cerr << "DrawImageDescriptors: " << DrawImageDescriptors << std::endl;
        std::cerr << "Image.DrawImage.ImageView: " << Image.DrawImage.ImageView << std::endl;

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            GPUSceneDataDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        

        constexpr size_t cascadeUBO = sizeof(ShadowCascadeUBO);
        constexpr size_t cascadeMatrices = sizeof(ShadowCascadeMatrices);
        fmt::println("Buffers size shadow: {}, {}", cascadeUBO, cascadeMatrices);
        for (size_t i = 0; i < FRAME_OVERLAP; i++)
        {
            /*GetFrameAt(i).FrameDescriptors = DescriptorAllocatorGrowable{};
            GetFrameAt(i).FrameDescriptors.Init(Device, 1000, frame_sizes);*/
            DescriptorWriter writer;
            writer.Clear();

            auto &globalDescriptor         = GetFrameAt(i).GlobalDescriptor;
            globalDescriptor       = GlobalDescriptorAllocator.Allocate(device, GPUSceneDataDescriptorLayout);
            writer.WriteBuffer(0, GetFrameAt(i).SceneDataBuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteImage(1, CascadeDepthImage.Image.ImageView, CascadeDepthImage.Sampler, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteBuffer(2, GetFrameAt(i).ShadowUBOBuffer.Buffer, cascadeUBO, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteBuffer(3, GetFrameAt(i).ShadowmMatrixBuffer.Buffer, cascadeMatrices, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.UpdateSet(device, globalDescriptor);

            std::cerr << "globalDescriptor: " << globalDescriptor << std::endl;

        }
    }

    void VkContextDevice::DestroyDescriptors()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        vkDestroyDescriptorSetLayout(device, DrawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, GPUSceneDataDescriptorLayout, nullptr);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).FrameDescriptors.DestroyPools(device);
        }
    }

    void VkContextDevice::InitSamples()
    {
        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;
        const VkDevice device = LogicalDevice::GetDevice();

        vkCreateSampler(device, &sampl, nullptr, &DefaultSamplerNearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;

        vkCreateSampler(device, &sampl, nullptr, &DefaultSamplerLinear);

    }

    void VkContextDevice::InitSwapchain(const VkExtent2D &inWindowExtent)
    {
        Swapchain.Init(*this, inWindowExtent, Image);

        
        
        InitShadowImages();

    }

    void VkContextDevice::ResizeSwapchain(const VkExtent2D& inWindowExtent)
    {
        const VkDevice device = LogicalDevice::GetDevice();

        vkDeviceWaitIdle(device);

        DestroyImages();

        Swapchain.Destroy(device);

        Swapchain.Init(*this, inWindowExtent, Image);
    }

    void VkContextDevice::InitShadowImages()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        const VkFormat         depthFormat = GetSupportedDepthFormat(true);
        const VkExtent3D shadowImageExtent{SHADOWMAP_DIMENSION, SHADOWMAP_DIMENSION, 1};
        auto imageInfo = vkinit::image_create_info(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImageExtent);

        imageInfo.extent.depth = 1;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = CASCADECOUNT;
        imageInfo.samples     = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling      = VK_IMAGE_TILING_OPTIMAL;

        constexpr auto allocinfo = VmaAllocationCreateInfo{
            .usage = VMA_MEMORY_USAGE_GPU_ONLY, 
            .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        };

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imageInfo, &allocinfo, &CascadeDepthImage.Image.Image, &CascadeDepthImage.Image.Allocation, &CascadeDepthImage.Image.Info));
        CascadeDepthImage.Image.ImageExtent = shadowImageExtent;
        CascadeDepthImage.Image.ImageFormat = depthFormat;
        {
            auto imageViewInfo = vkinit::imageview_create_info(
                    depthFormat, CascadeDepthImage.Image.Image, VK_IMAGE_ASPECT_DEPTH_BIT, CASCADECOUNT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

            imageViewInfo.subresourceRange.baseArrayLayer = 0;

            VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &CascadeDepthImage.Image.ImageView));
        }

        for (size_t i{ 0 }; i < CASCADECOUNT; i++)
        {
            /*auto imageViewInfo = vkinit::imageview_create_info(
                    VK_FORMAT_D32_SFLOAT, CascadeDepthImage.Image.Image, VK_IMAGE_ASPECT_DEPTH_BIT, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY);*/
            auto layerViewInfo =
                    vkinit::imageview_create_info(depthFormat,
                                                               CascadeDepthImage.Image.Image,
                                                               VK_IMAGE_ASPECT_DEPTH_BIT,
                                                               1,
                                                               VK_IMAGE_VIEW_TYPE_2D_ARRAY); 

            layerViewInfo.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
            //layerViewInfo.subresourceRange.layerCount     = 1;

            VK_CHECK(vkCreateImageView(device, &layerViewInfo, nullptr, &Cascades[i].View));

            fmt::println("Cascade[{}].View = {}", i, (uint64_t)Cascades[i].View);
        }

        VkSamplerCreateInfo samplerInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .maxAnisotropy = 1.f};
        samplerInfo.magFilter     = VK_FILTER_LINEAR;
        samplerInfo.minFilter     = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV  = samplerInfo.addressModeU;
        samplerInfo.addressModeW  = samplerInfo.addressModeU;
        samplerInfo.mipLodBias    = 0.0f;
        samplerInfo.maxAnisotropy = 1.0f;
        samplerInfo.minLod        = 0.0f;
        samplerInfo.maxLod        = 1.0f;
        samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &CascadeDepthImage.Sampler));
    }

    void VkContextDevice::InitTracyContext()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        #ifdef PROFILING
        for (size_t i{ 0 }; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).TracyContext = tracy::CreateVkContext(m_PhysicalDevice->GetDevice(),
                                                                device,
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

        for (auto& pickImage : PickingImages)
        {
            DestroyImage(pickImage);
        }
        //PickingImages.clear();
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

    VkFormat VkContextDevice::GetSupportedDepthFormat(bool checkSamplingSupport)
    {
        // All depth formats may be optional, so we need to find a suitable depth format to use
        constexpr std::array<VkFormat, 5> depthFormats = {
                VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};
        for (const auto &format : depthFormats)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice->GetDevice(), format, &formatProperties);
            // Format must support depth stencil attachment for optimal tiling
            if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            {
                if (checkSamplingSupport)
                {
                    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
                    {
                        continue;
                    }
                }
                return format;
            }
        }
        throw std::runtime_error("Could not find a matching depth format");
    }
}