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
#include "Graphics/ImGuiRenderer.h"
#include "Core/Log.h"
#include <VkBootstrap.h>

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = true;

    constexpr uint32_t BindlessTextureBinding = 10;
    constexpr uint32_t MAX_BINDLESS_RESOURCE = 1024;

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

        const auto instRet = builder.set_app_name("Mamont App").request_validation_layers(bUseValidationLayers)
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
                                                                 .sampleRateShading         = VK_TRUE,
                                                                 .multiDrawIndirect         = VK_TRUE,
                                                                 .drawIndirectFirstInstance = VK_TRUE,
                                                                 
                                                                 .depthClamp                = VK_TRUE,
                                                                 .depthBiasClamp            = VK_TRUE,
                                                                 .alphaToOne                = VK_TRUE,
                                                                 .samplerAnisotropy         = VK_TRUE, 
            
                                                                 .shaderInt64               = VK_TRUE};

         VkPhysicalDeviceFeatures2 deviceFeatures2{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = nullptr, .features = deviceFeatures};


        constexpr VkPhysicalDeviceVulkan14Features features14 = {
            .maintenance5 = VK_TRUE, .maintenance6 = VK_TRUE, .pushDescriptor = VK_TRUE
        };

        constexpr VkPhysicalDeviceVulkan13Features features = {
                .robustImageAccess = VK_TRUE,
                .inlineUniformBlock = VK_TRUE,
                .descriptorBindingInlineUniformBlockUpdateAfterBind = VK_TRUE,
                .synchronization2 = VK_TRUE,
                .dynamicRendering = VK_TRUE,
                .maintenance4 = VK_TRUE
        };
        
        constexpr VkPhysicalDeviceVulkan12Features features12 = {
                .uniformAndStorageBuffer8BitAccess = VK_TRUE,
                .shaderInt8                        = VK_TRUE,
                .descriptorIndexing                = VK_TRUE,
                .shaderUniformBufferArrayNonUniformIndexing = VK_TRUE,
                .shaderSampledImageArrayNonUniformIndexing  = VK_TRUE,
                .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
                .shaderStorageImageArrayNonUniformIndexing  = VK_TRUE,
                .shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE,
                .shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE,
                .shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE,
                .descriptorBindingSampledImageUpdateAfterBind  = VK_TRUE,
                .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
                .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
                .descriptorBindingUpdateUnusedWhilePending     = VK_TRUE,
                .descriptorBindingPartiallyBound               = VK_TRUE,
                .descriptorBindingVariableDescriptorCount      = VK_TRUE,
                .runtimeDescriptorArray                        = VK_TRUE,
                .scalarBlockLayout         = VK_TRUE,
#ifdef PROFILING
                .hostQueryReset = VK_TRUE,
#endif
                .timelineSemaphore = VK_TRUE,

                .bufferDeviceAddress       = VK_TRUE,
                .shaderOutputViewportIndex = VK_TRUE,
                
                .shaderOutputLayer         = VK_TRUE,
        };

        VkPhysicalDeviceRobustness2FeaturesKHR robustness2Features{};
        robustness2Features.sType               = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_KHR;
        robustness2Features.robustBufferAccess2 = VK_TRUE;
        robustness2Features.robustImageAccess2  = VK_TRUE;
        robustness2Features.nullDescriptor      = VK_TRUE;

        


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        std::vector<const char *> extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_ROBUSTNESS_2_EXTENSION_NAME, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME, 
            VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
        };

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
                                                           //.add_required_extension_features(robustness2Features)
                                                           .select()
                                                           .value();
        fmt::print("Name {}", physicalDevice.name);

        vkb::DeviceBuilder deviceBuilder{physicalDevice};

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

        InitImage();
        InitShadowImages();

        m_SkyboxTexture = std::unique_ptr<Texture>(LoadCubeMapTexture(DEFAULT_ASSETS_DIRECTORY + "Textures/cubemap_vulkan.ktx", VK_FORMAT_R8G8B8A8_UNORM));

        std::cerr << "-----m_SkyboxTexture: " << m_SkyboxTexture->Image << "\n";

        m_BRDFUTTexture = std::unique_ptr<Texture>(GenerateBRDFLUT());
        std::cerr << "-----m_BRDFUTTexture: " << m_BRDFUTTexture->Image << "\n";

        InitSceneBuffers();

        InitBindlessDescriptor();
        InitDescriptors();

        InitTracyContext();

        VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES, nullptr};
        VkPhysicalDeviceFeatures2                  device_features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexing_features};

        vkGetPhysicalDeviceFeatures2(PhysicalDevice::GetDevice(), &device_features);
        // For the feature to be correctly working, we need both the possibility to partially bind a descriptor,
        // as some entries in the bindless array will be empty, and SpirV runtime descriptors.
        bool bindlessSupported = indexing_features.descriptorBindingPartiallyBound && indexing_features.runtimeDescriptorArray;

        Log::Info("bindlessSupported: {}", bindlessSupported);

    }

    VkContextDevice::~VkContextDevice()
    {
        VkDevice device = LogicalDevice::GetDevice();

        DestroyBindlessDescriptor();

        m_SkyboxTexture.reset();
        m_IrradianceTexture.reset();
        m_BRDFUTTexture.reset();
        m_PrefilteredCubeTexture.reset();

        DestroyImages();
        DestroyShadowImages();

        Swapchain.Destroy(device);
        ImmediateContext::DestroyImmediateContext();
        
        vkDestroySurfaceKHR(Instance, Surface, nullptr);

        Allocator::Destroy();

        LogicalDevice::DestroyDevice();
        vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
        vkDestroyInstance(Instance, nullptr);
    }

    bool VkContextDevice::BeginFrame()
    {
        PROFILE_FUNCTION();

        auto           &currentFrame = GetCurrentFrame();
        const VkDevice &device       = LogicalDevice::GetDevice();

        if (m_AbsoluteFrame >= FRAME_OVERLAP)
        {
            PROFILE_ZONE("WaitSemaphores");

            const uint64_t waitValue = m_AbsoluteFrame - (FRAME_OVERLAP - 1);

            const VkSemaphoreWaitInfo waitInfo {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
                .semaphoreCount = 1,
                .pSemaphores    = &TimelineSemaphore,
                .pValues        = &waitValue
            };

            VK_CHECK(vkWaitSemaphores(device, &waitInfo, UINT64_MAX));
        }

        currentFrame.Deleteions.Flush();

        {
            PROFILE_ZONE("vkResetCommandPool");
            VK_CHECK(vkResetCommandPool(device, currentFrame.CommandPool, 0));
        }

        const auto [resultAcquire, swapchainImageIndex] = Swapchain.AcquireImage(device, currentFrame.SwapchainSemaphore);

        if (resultAcquire == VK_ERROR_OUT_OF_DATE_KHR || resultAcquire == VK_SUBOPTIMAL_KHR)
        {
            m_IsResizeRequested = true;
            return false;
        }

        return true;
    }

    bool VkContextDevice::EndFrame(VkCommandBuffer cmd)
    {
        PROFILE_FUNCTION();

        auto      &currentFrame        = GetCurrentFrame();
        const auto swapchainImageIndex = Swapchain.GetCurrentImageIndex();

        const uint64_t signalValue = ++TimelineSemaphoreValue; 

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);

        VkSemaphoreSubmitInfo waitInfo {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        waitInfo.semaphore = currentFrame.SwapchainSemaphore;
        waitInfo.value     = 0;
        waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signalTimeline {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalTimeline.semaphore = TimelineSemaphore;
        signalTimeline.value     = signalValue; 
        signalTimeline.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        VkSemaphoreSubmitInfo signalPresent {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
        signalPresent.semaphore = RenderCopleteSemaphores[swapchainImageIndex];
        signalPresent.value     = 0;
        signalPresent.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

        std::array<VkSemaphoreSubmitInfo, 2> signals = {signalTimeline, signalPresent};

        VkSubmitInfo2 submit{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
        submit.waitSemaphoreInfoCount   = 1;
        submit.pWaitSemaphoreInfos      = &waitInfo;
        submit.commandBufferInfoCount   = 1;
        submit.pCommandBufferInfos      = &cmdInfo;
        submit.signalSemaphoreInfoCount = static_cast<uint32_t>(signals.size());
        submit.pSignalSemaphoreInfos    = signals.data();

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, VK_NULL_HANDLE));

        const VkResult presentResult = Swapchain.Present(m_GraphicsQueue, &RenderCopleteSemaphores[swapchainImageIndex], swapchainImageIndex);

        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            m_IsResizeRequested = true;
            return false;
        }

        IncrementFrameNumber();
        return true;
    }

    void VkContextDevice::DestroyFrameData()
    {
        for (auto &frame : m_Frames)
        {
            frame.Deleteions.Flush();
        }

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
                vkinit::command_pool_create_info(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);
        const VkDevice device = LogicalDevice::GetDevice();

        for (auto& frame : m_Frames)
        {
            VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &frame.CommandPool));

            const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(frame.CommandPool, 1);
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &frame.MainCommandBuffer));
            const auto cmdBackgroundAllocInfo = vkinit::command_buffer_allocate_info(frame.CommandPool, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdBackgroundAllocInfo, &frame.BackgroundBuffer));
            
            const auto cmdSecondaryAllocInfo = vkinit::command_buffer_allocate_info(frame.CommandPool, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdSecondaryAllocInfo, &frame.UICommandBuffer));

            const auto cmdShadowAllocInfo = vkinit::command_buffer_allocate_info(frame.CommandPool, 1, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
            VK_CHECK(vkAllocateCommandBuffers(device, &cmdShadowAllocInfo, &frame.ShadowCommandBuffer));

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
        }

        RenderCopleteSemaphores.resize(Swapchain.GetImages().size());
        for (auto &semaphore : RenderCopleteSemaphores)
        {
            VK_CHECK(vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore));
        }
        
        const VkSemaphoreTypeCreateInfoKHR semaphoreTypeCreateInfo{
            .sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR,
            .initialValue  = TimelineSemaphoreValue
        };

        const VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeCreateInfo
        };

        VK_CHECK(vkCreateSemaphore(device, &semaphoreCI, nullptr, &TimelineSemaphore));

    }

    void VkContextDevice::DestroySyncStructeres()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        for (auto& frame : m_Frames)
        {
            vkDestroyFence(device, frame.RenderFence, nullptr);
            vkDestroySemaphore(device, frame.SwapchainSemaphore, nullptr);
        }
        
        for (auto &semaphore : RenderCopleteSemaphores)
        {
            vkDestroySemaphore(device, semaphore, nullptr);
        }

        vkDestroySemaphore(device, TimelineSemaphore, nullptr);
    }

    void VkContextDevice::InitSceneBuffers()
    {
        for (auto& frame : m_Frames)
        {
            frame.SceneDataBuffer.Create(sizeof(GPUSceneData),
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                                         VMA_MEMORY_USAGE_CPU_TO_GPU);
            const VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = frame.SceneDataBuffer.Buffer};
            frame.SceneDataBuffer.Address                     = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);
            
            frame.LightDataBuffer.Create(sizeof(LightData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

            //frame.CascadeMatrixBuffer.Create(sizeof(ShadowCascades), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }
    }

    void VkContextDevice::DestroySceneBuffers()
    {
        for (size_t i{0}; i < FRAME_OVERLAP; i++)
        {
            GetFrameAt(i).SceneDataBuffer.Destroy();
            GetFrameAt(i).LightDataBuffer.Destroy();
            //GetFrameAt(i).CascadeMatrixBuffer.Destroy();
        }
    }

    void VkContextDevice::InitDescriptors()
    {
        std::array<DescriptorAllocatorGrowable::PoolSizeRatio, 4> sizes = {
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4},
                DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 32}};
        const VkDevice device = LogicalDevice::GetDevice();

        GlobalDescriptorAllocator.Init(device, 100, sizes);

        { // Descriptor for Object (textures)
            DescriptorLayoutBuilder layoutBuilder{};
            layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5);

            RenderDescriptorLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            //builder.AddBinding(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
           
            builder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            builder.AddBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, PointLightShadowMaps.size());
            GPUSceneDataDescriptorLayout = builder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        std::cerr << "GPUSceneDataDescriptorLayout: " << GPUSceneDataDescriptorLayout << std::endl;
        std::cerr << "RenderDescriptorLayout: " << RenderDescriptorLayout << std::endl;

        constexpr size_t lightDataSize = sizeof(LightData);

        for (auto &frame : m_Frames)
        {
            DescriptorWriter writer;
            writer.Clear();

//TODO: CHANGE ORDER SETS

            auto &globalDescriptor = frame.GlobalDescriptor;
            globalDescriptor       = GlobalDescriptorAllocator.Allocate(device, GPUSceneDataDescriptorLayout);
            writer.WriteImage(0,
                              CascadeDepthImage.ImageView,
                              CascadeDepthImage.Sampler,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteBuffer(1, frame.LightDataBuffer.Buffer, lightDataSize, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            //writer.WriteBuffer(2, frame.CascadeMatrixBuffer.Buffer, sizeof(ShadowCascades), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteImage(2, m_SkyboxTexture->ImageView, m_SkyboxTexture->Sampler, 
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteImage(3,
                              m_BRDFUTTexture->ImageView,
                              m_BRDFUTTexture->Sampler, 
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            if (m_PrefilteredCubeTexture && m_PrefilteredCubeTexture->Image != VK_NULL_HANDLE && m_PrefilteredCubeTexture->Sampler != VK_NULL_HANDLE)
            {
                writer.WriteImage(4,
                                  m_PrefilteredCubeTexture->ImageView,
                                  m_PrefilteredCubeTexture->Sampler,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }
            if (m_IrradianceTexture && m_IrradianceTexture->Image != VK_NULL_HANDLE && m_IrradianceTexture->Sampler != VK_NULL_HANDLE)
            {
                writer.WriteImage(5,
                                  m_IrradianceTexture->ImageView,
                                  m_IrradianceTexture->Sampler,
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                              VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }

            std::vector<VkDescriptorImageInfo> pointInfos;
            pointInfos.reserve(MAX_POINT_LIGHT);

            for (const auto &shadowMap : PointLightShadowMaps)
            {
                VkDescriptorImageInfo info{};
                info.sampler     = shadowMap.Sampler;
                info.imageView   = shadowMap.ImageView;
                info.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                pointInfos.push_back(info);
            }
            writer.WriteImageArray(6, pointInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

            writer.UpdateSet(device, globalDescriptor);

            std::cerr << "globalDescriptor: " << globalDescriptor << std::endl;
        }
    }

    void VkContextDevice::DestroyDescriptors()
    {
        const VkDevice& device = LogicalDevice::GetDevice();

        GlobalDescriptorAllocator.DestroyPools(device);

        for (auto& frame : m_Frames)
        {
            frame.FrameDescriptors.DestroyPools(device);

            frame.GlobalDescriptor = VK_NULL_HANDLE;
            frame.ViewportDescriptor = VK_NULL_HANDLE;
        }

        vkDestroyDescriptorSetLayout(device, RenderDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, GPUSceneDataDescriptorLayout, nullptr);

        
    }

    void VkContextDevice::DestroyBindlessDescriptor()
    {
        const VkDevice &device = LogicalDevice::GetDevice();

        vkDestroyDescriptorSetLayout(device, BindlessDescriptorLayout, nullptr);
        vkDestroyDescriptorPool(device, BindlessDescriptorPool, nullptr);
    }

    void VkContextDevice::BindBindlessDescriptorSet(VkCommandBuffer cmd, VkPipelineLayout layout)
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &BindlessDescritporSet, 0, nullptr);
    }

    void VkContextDevice::UpdateBindlessDescriptorSet(const VkDescriptorImageInfo inImageInfo, uint32_t id)
    {
        VkWriteDescriptorSet write{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr, 
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        };
        write.dstBinding      = BindlessTextureBinding;
        write.descriptorCount = 1;
        write.dstSet          = BindlessDescritporSet;
        write.dstArrayElement = id;
        write.pImageInfo      = &inImageInfo;

        vkUpdateDescriptorSets(LogicalDevice::GetDevice(), 1, &write, 0, nullptr);
    }

    void VkContextDevice::InitSwapchain(const VkExtent2D &inWindowExtent)
    {
        Swapchain.Init(Surface, inWindowExtent);

        if (ImGuiRenderer::IsInitialize())
        {
            ImGuiIO &io = ImGui::GetIO();

            const auto &swapchainExtent = Swapchain.GetExtent();
            io.DisplaySize              = ImVec2(static_cast<float>(swapchainExtent.width), static_cast<float>(swapchainExtent.height));
        }

        //s_Window->InitVulkanWindow(Instance, PhysicalDevice::GetDevice(), LogicalDevice::GetDevice(), m_GraphicsQueueFamily);

    }

    void VkContextDevice::InitImage()
    {
        const VkExtent3D extent{Swapchain.GetExtent().width, Swapchain.GetExtent().height, 1};

        const auto &device = LogicalDevice::GetDevice();

        constexpr VkImageUsageFlags drawImageUsages =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        DrawImage.Create(extent, Swapchain.GetImageFormat() /*VK_FORMAT_B8G8R8A8_UNORM*/, drawImageUsages, true);

        //ImGuiView = Swapchain.GetImageView(0);

        std::cerr << "Image.DrawImage Image:" << DrawImage.Image << std::endl;
        std::cerr << "Image.DrawImage ImageView:" << DrawImage.ImageView << std::endl;

        constexpr VkImageUsageFlags depthImageUsages = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        DepthImage.Create(extent, Utils::FindDepthFormat(PhysicalDevice::GetDevice()) /*VK_FORMAT_D32_SFLOAT*/, depthImageUsages, false);
        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                { 
                VkUtil::transition_image(cmd, DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL); 
                VkUtil::transition_image(cmd, DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL); 
            });

        std::cerr << "Image.DepthImage Image:" << DepthImage.Image << std::endl;
        std::cerr << "Image.DepthImage ImageView:" << DepthImage.ImageView << std::endl;

        IdTexture.Create(extent, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter    = VK_FILTER_LINEAR;
        samplerInfo.minFilter    = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.minLod       = 0.0f;
        samplerInfo.maxLod       = 1.0f;
        VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &ImGuiSceneSampler));

        //InitShadowImages();
    }

    void VkContextDevice::ResizeSwapchain(const VkExtent2D& inNewExtent)
    {
        if (!m_IsResizeRequested)
            return;

        fmt::println("ResizeSwapchain");

        const VkDevice device = LogicalDevice::GetDevice();

        vkDeviceWaitIdle(device);

        DestroyImages();

        DestroyDescriptors();

        Swapchain.Destroy(device);

        Swapchain.Init(Surface, inNewExtent);

        InitImage();

        InitDescriptors();

        m_IsResizeRequested = false;
    }

    void VkContextDevice::InitShadowImages()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        // CascadeImage
        {
            const VkFormat       depthFormat = Utils::FindDepthFormat(PhysicalDevice::GetDevice());
            constexpr VkExtent3D shadowImageExtent{SHADOWMAP_DIMENSION, SHADOWMAP_DIMENSION, 1};

            auto imageInfo = vkinit::image_create_info(
                    depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImageExtent, CASCADECOUNT);

            imageInfo.mipLevels = 1;

            constexpr auto allocinfo = VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

            VK_CHECK(vmaCreateImage(
                    Allocator::GetAllocator(), &imageInfo, &allocinfo, &CascadeDepthImage.Image, &CascadeDepthImage.Allocation, &CascadeDepthImage.Info));

            CascadeDepthImage.ImageExtent = shadowImageExtent;
            CascadeDepthImage.ImageFormat = depthFormat;

            auto imageViewInfo =
                    vkinit::imageviewCreateInfo(depthFormat, CascadeDepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT, 1, CASCADECOUNT, VK_IMAGE_VIEW_TYPE_2D_ARRAY);

            VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &CascadeDepthImage.ImageView));

            ImmediateContext::ImmediateSubmit(
                    [&](VkCommandBuffer cmd)
                    { VkUtil::transition_image(cmd, CascadeDepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL); });
            
        }

        std::cerr << "Shadow Image:" << CascadeDepthImage.Image << std::endl;
        std::cerr << "Shadow ImageView:" << CascadeDepthImage.ImageView << std::endl;

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
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;

            VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &CascadeDepthImage.Sampler));
        }

        std::cerr << "Start Creating Point Image" << std::endl;
        // Create Point Shadow Image
        PointShadowMapImageViews.resize(SHADOW_FACE_NUM * MAX_POINT_LIGHT);
        uint32_t j = 0;
        const VkFormat pointDepthFormat = Utils::FindDepthFormat(PhysicalDevice::GetDevice());
        constexpr VkExtent3D shadowImageExtent{SHADOW_MAP_FACE_IMAGE_SIZE, SHADOW_MAP_FACE_IMAGE_SIZE, 1};

        for (auto& shadowMap : PointLightShadowMaps)
        {
            auto imageInfo = vkinit::image_create_info(pointDepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, shadowImageExtent);

            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

            imageInfo.mipLevels   = 1;
            imageInfo.arrayLayers = SHADOW_FACE_NUM * MAX_POINT_LIGHT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            constexpr auto allocinfo = VmaAllocationCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY, .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT};

            VK_CHECK(vmaCreateImage(Allocator::GetAllocator(), &imageInfo, &allocinfo, &shadowMap.Image, &shadowMap.Allocation, &shadowMap.Info));

            /* ImmediateContext::ImmediateSubmit(
                    [&](VkCommandBuffer cmd)
                    { 
                VkUtil::transition_image(cmd, shadowMap.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1, 6);
            });*/

            shadowMap.ImageExtent   = shadowImageExtent;
            shadowMap.ImageFormat = pointDepthFormat;

            auto imageViewInfo = vkinit::imageviewCreateInfo(pointDepthFormat, shadowMap.Image, VK_IMAGE_ASPECT_DEPTH_BIT, 0, 6, VK_IMAGE_VIEW_TYPE_CUBE);
            imageViewInfo.subresourceRange = {VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 6};

            VK_CHECK(vkCreateImageView(device, &imageViewInfo, nullptr, &shadowMap.ImageView));

            for (uint32_t i = 0; i < SHADOW_FACE_NUM; ++i)
            {
                const auto viewCreateInfo = VkImageViewCreateInfo{
                        .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                        .image    = shadowMap.Image,
                        .viewType = VK_IMAGE_VIEW_TYPE_2D,
                        .format   = pointDepthFormat,
                        .subresourceRange =
                                VkImageSubresourceRange{
                                        .aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
                                        .baseMipLevel   = 0,
                                        .levelCount     = 1,
                                        .baseArrayLayer = i,
                                        .layerCount     = 1,
                                },
                };

                VK_CHECK(vkCreateImageView(device, &viewCreateInfo, nullptr, &PointShadowMapImageViews[i + j * SHADOW_FACE_NUM]));
            }

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
            samplerInfo.maxLod        = 0.0f;
            samplerInfo.compareEnable = VK_TRUE;
            samplerInfo.compareOp     = VK_COMPARE_OP_LESS_OR_EQUAL;
            //samplerInfo.compareOp     = VK_COMPARE_OP_NEVER;
            //samplerInfo.borderColor   = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

            VK_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &shadowMap.Sampler));
            std::cerr << "Point Image: " << shadowMap.Image << std::endl;

            j++;
        }
        
    }

    void VkContextDevice::CreatePrefilteredCubeTexture(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &&inDrawSkyboxFunc)
    {
        m_PrefilteredCubeTexture = std::unique_ptr<Texture>(GeneratePrefilteredCube(vertexAddress, inDrawSkyboxFunc, *m_SkyboxTexture));
        m_IrradianceTexture      = std::unique_ptr<Texture>(GenerateIrradianceTexture(vertexAddress, inDrawSkyboxFunc, *m_SkyboxTexture));
        std::cerr << "-----m_PrefilteredCubeTexture: " << m_PrefilteredCubeTexture->Image << "\n";
        std::cerr << "-----m_IrradianceTexture: " << m_IrradianceTexture->Image << "\n";

        DestroyDescriptors();
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

    void VkContextDevice::InitBindlessDescriptor()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        constexpr std::array<VkDescriptorPoolSize, 2> poolSizedBindless = {
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_RESOURCE},
                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_RESOURCE}};

        VkDescriptorPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .pNext = nullptr};
        poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        poolInfo.maxSets                    = MAX_BINDLESS_RESOURCE * static_cast<uint32_t>(poolSizedBindless.size());
        poolInfo.poolSizeCount              = poolSizedBindless.size();
        poolInfo.pPoolSizes                 = poolSizedBindless.data();

        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &BindlessDescriptorPool));

        const uint32_t                              poolCount = static_cast<uint32_t>(poolSizedBindless.size());
        std::array<VkDescriptorSetLayoutBinding, 4> bindings{};

        bindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount    = MAX_BINDLESS_RESOURCE;
        bindings[0].binding            = BindlessTextureBinding;
        bindings[0].stageFlags         = VK_SHADER_STAGE_ALL;
        bindings[0].pImmutableSamplers = nullptr;

        bindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        bindings[1].descriptorCount    = MAX_BINDLESS_RESOURCE;
        bindings[1].binding            = BindlessTextureBinding + 1;
        bindings[1].stageFlags         = VK_SHADER_STAGE_ALL;
        bindings[1].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        layoutInfo.bindingCount                    = poolCount;
        layoutInfo.pBindings                       = bindings.data();
        layoutInfo.flags                           = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

        VkDescriptorBindingFlags bindlessFlags = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                                                 /*VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |*/ VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;
        std::array<VkDescriptorBindingFlags, 4> binding_flags{};

        binding_flags[0] = bindlessFlags;
        binding_flags[1] = bindlessFlags;

        const VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
                                                                          .pNext         = nullptr,
                                                                          .bindingCount  = poolCount,
                                                                          .pBindingFlags = binding_flags.data()};

        layoutInfo.pNext = &extendedInfo;

        VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &BindlessDescriptorLayout));
        VkDescriptorSetAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .pNext = nullptr};
        allocInfo.descriptorPool     = BindlessDescriptorPool;
        allocInfo.pSetLayouts        = &BindlessDescriptorLayout;
        allocInfo.descriptorSetCount = 1;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &BindlessDescritporSet));
    }

    void VkContextDevice::DestroyImages()
    {
        DrawImage.Destroy();
        DepthImage.Destroy();

        IdTexture.Destroy();

        vkDestroySampler(LogicalDevice::GetDevice(), ImGuiSceneSampler, nullptr);

    }

    void VkContextDevice::DestroyShadowImages()
    {
        CascadeDepthImage.Destroy();

        const auto &device = LogicalDevice::GetDevice();

        for (auto &imageview : PointShadowMapImageViews)
        {
            vkDestroyImageView(device, imageview, nullptr);
        }

        for (auto &shadowMap : PointLightShadowMaps)
        {
            shadowMap.Destroy();
        }
    }

    FrameData &VkContextDevice::GetCurrentFrame()
    {
        return m_Frames.at(m_FrameNumber);
    }

    const FrameData &VkContextDevice::GetCurrentFrame() const
    {
        return m_Frames.at(m_FrameNumber);
    }

    void VkContextDevice::IncrementFrameNumber()
    {
        m_FrameNumber = (m_FrameNumber + 1) % FRAME_OVERLAP;
        ++m_AbsoluteFrame;
    }

    VkFormat VkContextDevice::GetSupportedDepthFormat(bool checkSamplingSupport)
    {
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
