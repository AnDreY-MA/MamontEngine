#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#include "GLFW/glfw3.h"

#include "Engine.h"
#include "Core/Base.h"
#include "Core/Window.h"
#include "Window.h"
#include <iostream>
#include <set>


namespace MamontEngine
{
    Engine* Engine::s_Instance = nullptr;
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
                                            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                            VkDebugUtilsMessageTypeFlagsEXT messageType,
                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                            void* pUserData) 
    {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    #ifdef NDEBUG
        const bool enableValidationLayers = false;
    #else
        const bool enableValidationLayers = true;
    #endif

    Engine::Engine()
    {
        s_Instance = this;

        Init();
    }
    Engine::~Engine()
    {
        std::cout << "~Engine" << std::endl;
        vkDestroyDevice(m_Device, nullptr);
        if (enableValidationLayers)
        {
            DestroyDebugUtilsMessengerEXT(m_VkInstance, m_debugMessenger, nullptr);
        }
        vkDestroySurfaceKHR(m_VkInstance, m_Surface, nullptr);
        vkDestroyInstance(m_VkInstance,nullptr);
    }

    void Engine::Init()
    {
        std::cout << "Init" << std::endl;

        m_Window = Window::Create();
        m_Window->SetEventCallback(MM_BIND_EVENT_FN(Engine::OnEvent));

        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
    }

    void Engine::Run()
    {
        VkFence closeEvent;
        //vkCreateFence(m_Device, nullptr, nullptr, &closeEvent);
      
        while(m_Running)
        {
            
            /* VkResult closeResult = vkGetFenceStatus(m_Device, closeEvent);
            if (closeResult == VK_SUCCESS)
            {
                m_Running = false;
            }*/
            m_Window->OnUpdate();
        }

        std::cout << "EndRun" << std::endl;
    }

    void Engine::OnEvent(Event& event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowCloseEvent>(MM_BIND_EVENT_FN(Engine::OnWindowClose));
        dispatcher.Dispatch<WindowCloseEvent>(MM_BIND_EVENT_FN(Engine::OnWindowClose));
    }

    bool Engine::OnWindowClose(WindowCloseEvent& e)
    {
        std::cout << "FALSE" << std::endl;
        
        m_Running = false;
        return true;

    }
    bool Engine::OnWindowResize(WindowResizeEvent& e)
    {
        return false;
    }

    void Engine::CreateInstance()
    {
        if(enableValidationLayers && !CheckValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Triagle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Mamont Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount {0};
        const char** glfwExtenstions;
        glfwExtenstions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        auto extensions = GetRequiredExtensions();

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount = 0;

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) 
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            PopulateDebugMessengerCreateInfor(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        }
        else 
        {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        } 

        if(vkCreateInstance(&createInfo, nullptr, &m_VkInstance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create instance!");
        }
    }
    
    bool Engine::IsDeviceSuitable(VkPhysicalDevice inDevice)
    {
        QueueFamilyIndices indices = FindQueueFamilies(inDevice);

        return indices.IsComplete();
    }
    
    void Engine::PickPhysicalDevice()
    {
        uint32_t deviceCount{0};
        vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("Failed to find GPU's with Vulkan support!");
        }


        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_VkInstance, &deviceCount, devices.data());

        for (const auto& device : devices)
        {
            if(IsDeviceSuitable(device))
            {
                m_physicalDevice = device;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("Failed to find a suitable GPU!"); 
        }
    }

    void Engine::CreateLogicalDevice()
    {
        QueueFamilyIndices                   indices = FindQueueFamilies(m_physicalDevice);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t>                   uniqueQueueFamilies = {indices.GraphicsFamily.value(), indices.presentFamily.value()};

        float queuePriority{1.f};
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount       = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures     = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create logical device!");
        }
        
        vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_graphicQueue);
        vkGetDeviceQueue(m_Device, indices.presentFamily.value(), 0, &m_PresentQueue);
    }

    QueueFamilyIndices Engine::FindQueueFamilies(VkPhysicalDevice inDevice)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount{0};
        vkGetPhysicalDeviceQueueFamilyProperties(inDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(inDevice, &queueFamilyCount, queueFamilies.data());

        int index {0};
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.GraphicsFamily = index;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(inDevice, index, m_Surface, &presentSupport);

            if (presentSupport)
                indices.presentFamily = index;
            
            if(indices.IsComplete())
                break;

            index++;
        }

        return indices;
    }

    VkResult Engine::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
            const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if(func != nullptr)
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void Engine::PopulateDebugMessengerCreateInfor(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =  VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
            | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        createInfo.pfnUserCallback = DebugCallback;
        //createInfo.pUserData = nullptr;
    }

    void Engine::SetupDebugMessenger()
    {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        PopulateDebugMessengerCreateInfor(createInfo);

        if(CreateDebugUtilsMessengerEXT(m_VkInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to set up debug messenger!");
        }

    }

    void Engine::CreateSurface()
    {
        if (glfwCreateWindowSurface(m_VkInstance, m_Window->GetGLFWWindow(), nullptr, &m_Surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
    }
    void Engine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    bool Engine::CheckValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount,nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers)
        {
            bool layerFound{false};

            for (const auto& layerProperties : availableLayers)
            {
                if(strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if(!layerFound)
                return false;
            
        }

        return true;
    }

    std::vector<const char*> Engine::GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount{0};
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if(enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    

    void Engine::Loop()
    {
        
    }

    void Engine::Cleanup()
    {

    }
}