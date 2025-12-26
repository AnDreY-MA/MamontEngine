#include "ContextDevice.h"

#include <glm/gtx/transform.hpp>
#include <vk_mem_alloc.h>
#include "ECS/SceneRenderer.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Utils/Utilities.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include "Window.h"
#include "tracy/TracyVulkan.hpp"
#include "vulkan/vk_enum_string_helper.h"
#include "Graphics/Vulkan/ImmediateContext.h"

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;

    static VKAPI_ATTR VkBool32 VKAPI_CALL DetailedDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
                                                                VkDebugUtilsMessageTypeFlagsEXT             type,
                                                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                                void                                       *pUserData)
    {

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

        if (pCallbackData->queueLabelCount > 0)
        {
            std::cerr << "Queue labels:" << std::endl;
            for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
            {
                std::cerr << "  - " << pCallbackData->pQueueLabels[i].pLabelName << std::endl;
            }
        }

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
        // DebugMessenger                  = vkbInstance.debug_messenger;
        if (vkbInstance.allocation_callbacks == nullptr)
        {
            fmt::println("allocation_callbacks == nullptr");
        }

        if (!Instance || !DebugMessenger)
        {
            fmt::println("m_ContextDevice.Instance || m_ContextDevice.DebugMessenger is NULL");
        }

        inWindow->CreateSurface(Instance, &Surface);

        constexpr auto deviceFeatures = VkPhysicalDeviceFeatures{.imageCubeArray            = VK_TRUE,
                                                                 .geometryShader            = VK_TRUE,
                                                                 .multiDrawIndirect         = VK_TRUE,
                                                                 .drawIndirectFirstInstance = VK_TRUE,
                                                                 .depthClamp                = VK_TRUE,
                                                                 .depthBiasClamp            = VK_TRUE,
                                                                 .samplerAnisotropy         = VK_TRUE,
                                                                 .shaderInt64               = VK_TRUE};

        constexpr VkPhysicalDeviceVulkan14Features features14 = {.maintenance5 = VK_TRUE, .maintenance6 = VK_TRUE, .pushDescriptor = VK_TRUE};

        constexpr VkPhysicalDeviceVulkan13Features features = {
                .synchronization2 = VK_TRUE,
                .dynamicRendering = VK_TRUE,
        };

        constexpr VkPhysicalDeviceVulkan12Features features12 = {
                .uniformAndStorageBuffer8BitAccess            = VK_TRUE,
                .shaderInt8                                   = VK_TRUE,
                .descriptorIndexing                           = VK_TRUE,
                .shaderSampledImageArrayNonUniformIndexing    = VK_TRUE,
                .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
                .descriptorBindingUpdateUnusedWhilePending    = VK_TRUE,
                .descriptorBindingPartiallyBound              = VK_TRUE,
                .descriptorBindingVariableDescriptorCount     = VK_TRUE,
                .runtimeDescriptorArray                       = VK_TRUE,
#ifdef PROFILING
                .hostQueryReset = VK_TRUE,
#endif
                .timelineSemaphore   = VK_TRUE,
                .bufferDeviceAddress = VK_TRUE,
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
        PhysicalDevice::Init(physicalDevice.physical_device);

        m_GraphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        const VmaAllocatorCreateInfo allocatorInfo = {.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
                                                      .physicalDevice = PhysicalDevice::GetDevice(),
                                                      .device         = LogicalDevice::GetDevice(),
                                                      .instance       = Instance};
        Allocator::Init(allocatorInfo);

        InitSwapchain(inWindow->GetExtent());

        InitCommands();

        InitSyncStructeres();

        ImmediateContext::InitImmediateContext(m_GraphicsQueue, m_GraphicsQueueFamily);

        m_SkyboxTexture = LoadCubeMapTexture(DEFAULT_ASSETS_DIRECTORY + "Textures/cubemap_vulkan.ktx", VK_FORMAT_R8G8B8A8_UNORM);

        m_BRDFUTTexture = GenerateBRDFLUT();

        InitSceneBuffers();

        InitDescriptors();

        InitTracyContext();
    }

    VkContextDevice::~VkContextDevice()
    {
        m_SkyboxTexture.Destroy();
        m_BRDFUTTexture.Destroy();
        m_PrefilteredCubeTexture.Destroy();
        m_IrradianceTexture.Destroy();

        VkDevice device = LogicalDevice::GetDevice();
        Swapchain.Destroy(device);
        ImmediateContext::DestroyImmediateContext();
        
        vkDestroySurfaceKHR(Instance, Surface, nullptr);

        Allocator::Destroy();

        LogicalDevice::DestroyDevice();
        vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
        vkDestroyInstance(Instance, nullptr);
    }

    void VkContextDevice::DestroyFrameData()
    {
        for (auto &frame : m_Frames)
        {
            frame.Deleteions.Flush();
        }

        DestroyImages();
        DestroyCommands();
        DestroySyncStructeres();
        DestroyDescriptors();
        DestroySceneBuffers();

#ifdef PROFILING
        for (size_t i{0}; i < FRAME_OVERLAP; ++i)
        {
            tracy::DestroyVkContext(GetFrameAt(i).TracyContext);
        }
#endif
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

            // VK_CHECK(vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &GetFrameAt(i).RenderSemaphore));
        }

        RenderCopleteSemaphores.resize(Swapchain.GetImages().size());
        for (auto &semaphore : RenderCopleteSemaphores)
        {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));
        }

        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &TransferFence));

    }

    void VkContextDevice::DestroySyncStructeres()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        vkDestroyFence(device, TransferFence, nullptr);

        for (size_t i = 0; i < FRAME_OVERLAP; ++i)
        {
            vkDestroyFence(device, GetFrameAt(i).RenderFence, nullptr);
            vkDestroySemaphore(device, GetFrameAt(i).SwapchainSemaphore, nullptr);
            // vkDestroySemaphore(Device, GetFrameAt(i).RenderSemaphore, nullptr);
        }
        for (auto &semaphore : RenderCopleteSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }
    }

    void VkContextDevice::InitSceneBuffers()
    {
        for (size_t i{0}; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).SceneDataBuffer.Create(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            GetFrameAt(i).CascadeDataBuffer.Create(sizeof(CascadeData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            GetFrameAt(i).CascadeMatrixBuffer.Create(sizeof(glm::mat4) * CASCADECOUNT, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    void VkContextDevice::DestroySceneBuffers()
    {
        for (size_t i{0}; i < FRAME_OVERLAP; i++)
        {
            GetFrameAt(i).SceneDataBuffer.Destroy();
            GetFrameAt(i).CascadeDataBuffer.Destroy();
            GetFrameAt(i).CascadeMatrixBuffer.Destroy();
            /*GetFrameAt(i).ShadowMatrixBuffer.Destroy();
            GetFrameAt(i).ShadowUBOBuffer.Destroy();*/
        }
    }

    void VkContextDevice::InitDescriptors()
    {
        std::array<DescriptorAllocatorGrowable::PoolSizeRatio, 4> sizes = {
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4}};
        const VkDevice device = LogicalDevice::GetDevice();

        GlobalDescriptorAllocator.Init(device, 100, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            DrawImageDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_COMPUTE_BIT);

            DrawImageDescriptors = GlobalDescriptorAllocator.Allocate(device, DrawImageDescriptorLayout);
        }

        {
            DescriptorLayoutBuilder layoutBuilder{};
            layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            layoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            layoutBuilder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            layoutBuilder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            RenderDescriptorLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        {
            DescriptorWriter writer;
            writer.WriteImage(0, DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            writer.UpdateSet(device, DrawImageDescriptors);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            builder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            GPUSceneDataDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        constexpr size_t cascadeDataSize     = sizeof(CascadeData);
        constexpr size_t cascadeMatricesSize = sizeof(glm::mat4) * CASCADECOUNT;

        for (auto &frame : m_Frames)
        {
            /*GetFrameAt(i).FrameDescriptors = DescriptorAllocatorGrowable{};
            GetFrameAt(i).FrameDescriptors.Init(Device, 1000, frame_sizes);*/
            DescriptorWriter writer;
            writer.Clear();

            auto &globalDescriptor = frame.GlobalDescriptor;
            globalDescriptor       = GlobalDescriptorAllocator.Allocate(device, GPUSceneDataDescriptorLayout);
            writer.WriteBuffer(0, frame.SceneDataBuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteImage(1,
                              CascadeDepthImage.ImageView,
                              CascadeDepthImage.Sampler,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteBuffer(2, frame.CascadeDataBuffer.Buffer, cascadeDataSize, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteBuffer(3, frame.CascadeMatrixBuffer.Buffer, cascadeMatricesSize, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteImage(4, m_SkyboxTexture.ImageView, m_SkyboxTexture.Sampler, 
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteImage(
                    5, m_BRDFUTTexture.ImageView, m_BRDFUTTexture.Sampler, 
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            if (m_PrefilteredCubeTexture.Image != VK_NULL_HANDLE && m_PrefilteredCubeTexture.Sampler != VK_NULL_HANDLE)
            {
                writer.WriteImage(6,
                              m_PrefilteredCubeTexture.ImageView,
                              m_PrefilteredCubeTexture.Sampler,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }
            if (m_IrradianceTexture.Image != VK_NULL_HANDLE && m_IrradianceTexture.Sampler != VK_NULL_HANDLE)
            {
                writer.WriteImage(7,
                                  m_IrradianceTexture.ImageView,
                                  m_IrradianceTexture.Sampler,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }
            
            
            writer.UpdateSet(device, globalDescriptor);

            std::cerr << "globalDescriptor: " << globalDescriptor << std::endl;
        }
    }

    void VkContextDevice::DestroyDescriptors()
    {
        VkDevice device = LogicalDevice::GetDevice();

        vkDestroyDescriptorSetLayout(device, DrawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, GPUSceneDataDescriptorLayout, nullptr);

        DrawImageDescriptors = VK_NULL_HANDLE;

        for (auto& frame : m_Frames)
        {
            frame.GlobalDescriptor = VK_NULL_HANDLE;
            frame.ViewportDescriptor = VK_NULL_HANDLE;
            frame.FrameDescriptors.DestroyPools(device);
        }
    }

    void VkContextDevice::InitSwapchain(const VkExtent2D &inWindowExtent)
    {
        Swapchain.Init(Surface, inWindowExtent);
        InitImage();
    }

    void VkContextDevice::InitImage()
    {
        const VkExtent3D extent{Swapchain.GetExtent().width, Swapchain.GetExtent().height, 1};

        constexpr VkImageUsageFlags drawImageUsages = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        DrawImage.Create(extent, Swapchain.GetImageFormat() /*VK_FORMAT_B8G8R8A8_UNORM*/, drawImageUsages, true);

        std::cerr << "Image.DrawImage Image:" << DrawImage.Image << std::endl;
        std::cerr << "Image.DrawImage ImageView:" << DrawImage.ImageView << std::endl;

        constexpr VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        DepthImage.Create(extent, Utils::FindDepthFormat(PhysicalDevice::GetDevice()) /*VK_FORMAT_D32_SFLOAT*/, depthImageUsages, false);

        std::cerr << "Image.DepthImage Image:" << DepthImage.Image << std::endl;
        std::cerr << "Image.DepthImage ImageView:" << DepthImage.ImageView << std::endl;

        {
            const VkExtent3D extent = {Swapchain.GetExtent().width, Swapchain.GetExtent().height, 1};

            PickingImages.resize(Swapchain.GetImages().size());
            for (auto &image : PickingImages)
            {
                image.Create(
                        extent, VK_FORMAT_R32G32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

                std::cerr << "PickingImages Image:" << image.Image << std::endl;
                std::cerr << "PickingImages ImageView:" << image.ImageView << std::endl;
            }
        }

        InitShadowImages();
    }

    void VkContextDevice::ResizeSwapchain(const VkExtent2D &inWindowExtent)
    {
        const VkDevice device = LogicalDevice::GetDevice();

        vkDeviceWaitIdle(device);

        DestroyImages();

        Swapchain.ReCreate(Surface, inWindowExtent);

        InitImage();

        InitDescriptors();
    }

    void VkContextDevice::InitShadowImages()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        const VkFormat       depthFormat = Utils::FindDepthFormat(PhysicalDevice::GetDevice());
        constexpr VkExtent3D shadowImageExtent{SHADOWMAP_DIMENSION, SHADOWMAP_DIMENSION, 1};

        auto imageInfo = vkinit::image_create_info(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImageExtent);

        imageInfo.mipLevels   = 1;
        imageInfo.arrayLayers = CASCADECOUNT;

        constexpr auto allocinfo = VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

        VK_CHECK(vmaCreateImage(Allocator::GetAllocator(),
                                &imageInfo,
                                &allocinfo,
                                &CascadeDepthImage.Image,
                                &CascadeDepthImage.Allocation,
                                &CascadeDepthImage.Info));

        CascadeDepthImage.ImageExtent = shadowImageExtent;
        CascadeDepthImage.ImageFormat = depthFormat;

        {
            auto imageViewInfo = vkinit::imageviewCreateInfo(
                    depthFormat, CascadeDepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT, 1, CASCADECOUNT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

            VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &CascadeDepthImage.ImageView));
        }

        std::cerr << "Shadow Image:" << CascadeDepthImage.Image << std::endl;
        std::cerr << "Shadow ImageView:" << CascadeDepthImage.ImageView << std::endl;

        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            auto layerViewInfo =
                    vkinit::imageviewCreateInfo(depthFormat, CascadeDepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT, 1, 1, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

            layerViewInfo.subresourceRange.baseArrayLayer = static_cast<uint32_t>(i);
            layerViewInfo.subresourceRange.layerCount     = 1;

            VK_CHECK(vkCreateImageView(device, &layerViewInfo, nullptr, &Cascades[i].View));
            std::cerr << "Cascades[" << i << "].View: " << Cascades[i].View << std::endl;
        }

        // Cascade Sampler
        {
            VkSamplerCreateInfo samplerInfo{.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
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
    }

    void VkContextDevice::CreatePrefilteredCubeTexture(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &&inDrawSkyboxFunc)
    {
        m_PrefilteredCubeTexture = GeneratePrefilteredCube(vertexAddress, inDrawSkyboxFunc, m_SkyboxTexture);
        m_IrradianceTexture = GenerateIrradianceTexture(vertexAddress, inDrawSkyboxFunc, m_SkyboxTexture);
        
        InitDescriptors();
    }

    void VkContextDevice::InitTracyContext()
    {
        const VkDevice device = LogicalDevice::GetDevice();

#ifdef PROFILING
        for (size_t i{0}; i < FRAME_OVERLAP; ++i)
        {
            GetFrameAt(i).TracyContext = tracy::CreateVkContext(PhysicalDevice::GetDevice(),
                                                                device,
                                                                m_GraphicsQueue,
                                                                m_TracyInfo.CommandBuffer,
                                                                m_TracyInfo.GetPhysicalDeviceCalibrateableTimeDomainsEXT,
                                                                m_TracyInfo.GetCalibratedTimestampsEXT);

            char      buf[50];
            const int n = sprintf(buf, "Vulkan Context [%d]", i);
            TracyVkContextName(GetFrameAt(i).TracyContext, buf, n);
        }
#endif
    }

    void VkContextDevice::DestroyImages()
    {
        DrawImage.Destroy();
        DepthImage.Destroy();

        for (auto &pickImage : PickingImages)
        {
            pickImage.Destroy();
        }
        CascadeDepthImage.Destroy();
    }

    FrameData &VkContextDevice::GetCurrentFrame()
    {
        return m_Frames.at(m_FrameNumber);
    }

    const FrameData &VkContextDevice::GetCurrentFrame() const
    {
        return m_Frames.at(m_FrameNumber);
    }

    VkFormat VkContextDevice::GetSupportedDepthFormat(bool checkSamplingSupport)
    {
        // All depth formats may be optional, so we need to find a suitable depth format to use
        constexpr std::array<VkFormat, 5> depthFormats = {
                VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM};
        for (const auto &format : depthFormats)
        {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(PhysicalDevice::GetDevice(), format, &formatProperties);
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

} // namespace MamontEngine
