#include "MDevice.h"
#include <VkBootstrap.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL.h>
#include "Core/VkInitializers.h"
#include <Core/RenderData.h>


namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;

    
    void MDevice::Init(SDL_Window *inWindow)
    {
        vkb::InstanceBuilder builder;

        auto instRet = builder.set_app_name("Mamont App")
                               .request_validation_layers(bUseValidationLayers)
                               .use_default_debug_messenger()
                               .require_api_version(1, 3, 290)
                               .build();

        vkb::Instance vkbInstance = instRet.value();
        m_Instance                = vkbInstance.instance;
        m_DebugMessenger          = vkbInstance.debug_messenger;

        if (!m_Instance || !m_DebugMessenger)
        {
            fmt::println("m_Instance || m_DebugMessenger is NULL");
        }

        if (!SDL_Vulkan_CreateSurface(inWindow, m_Instance, nullptr, &m_Surface))
        {
            fmt::print("Surface is false");
            return;
        }

        VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features.dynamicRendering = true;
        features.synchronization2 = true;

        VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features12.bufferDeviceAddress               = true;
        features12.descriptorIndexing                = true;
        features12.shaderInt8                        = true;
        features12.uniformAndStorageBuffer8BitAccess = true;
        features12.runtimeDescriptorArray            = true;


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                                     .set_required_features_13(features)
                                                     .set_required_features_12(features12)
                                                     .set_surface(m_Surface)
                                                     //.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
                                                     .select()
                                                     .value();

        vkb::DeviceBuilder deviceBuilder{physicalDevice};

        vkb::Device vkbDevice = deviceBuilder.build().value();

        m_Device    = vkbDevice.device;
        m_ChosenGPU = physicalDevice.physical_device;
        fmt::print("Name {}", physicalDevice.name);

        m_GraphicsQueue       = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
        VmaVulkanFunctions vulkanFunctions    = {};
        vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = m_ChosenGPU;
        allocatorInfo.device                 = m_Device;
        allocatorInfo.instance               = m_Instance;
        allocatorInfo.pVulkanFunctions       = &vulkanFunctions;
        allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT | VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
        allocatorInfo.pAllocationCallbacks   = nullptr;     
        vmaCreateAllocator(&allocatorInfo, &m_Allocator);

        //m_MainDeletionQueue.PushFunction([&]() { vmaDestroyAllocator(m_Allocator); });
    }

    void MDevice::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& inFunction) const
    {
        VK_CHECK(vkResetFences(m_Device, 1, &m_ImmFence));
        VK_CHECK(vkResetCommandBuffer(m_ImmCommandBuffer, 0));

        VkCommandBuffer cmd = m_ImmCommandBuffer;

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        inFunction(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_ImmFence));

        VK_CHECK(vkWaitForFences(m_Device, 1, &m_ImmFence, true, 9999999999));
    }

    void MDevice::Shutdown() const
    {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
        vmaDestroyAllocator(m_Allocator);

        vkDestroyDevice(m_Device, nullptr);
        vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
        vkDestroyInstance(m_Instance, nullptr);
    }
    void MDevice::DestroyImage(const AllocatedImage &inImage) const
    {
        vkDestroyImageView(m_Device, inImage.ImageView, nullptr);
        vmaDestroyImage(m_Allocator, inImage.Image, inImage.Allocation);
    }

}