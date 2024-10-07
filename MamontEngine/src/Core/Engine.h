#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "GLFW/glfw3.h"

namespace MamontEngine
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> GraphicsFamily;
        std::optional<uint32_t> presentFamily;
        
        const bool IsComplete() const { return GraphicsFamily.has_value(); }
    };

    class Engine 
    {
    public:
        Engine();
        virtual ~Engine();
        
        void Run();

        static Engine& Get()
        {
            return *s_Instance;
        }

    private:
        void Init();
        void Loop();
        void Cleanup();

        void OnEvent(Event& event);

        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        void CreateInstance();
        void SetupDebugMessenger();
        void CreateSurface();
        void PickPhysicalDevice();

        void CreateLogicalDevice();

        bool IsDeviceSuitable(VkPhysicalDevice inDevice);

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice inDevice);
        
        void PopulateDebugMessengerCreateInfor(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
            const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        bool CheckValidationLayerSupport();

        std::vector<const char*> GetRequiredExtensions();

    private:
        bool m_Running{true};

        VkInstance m_VkInstance;
        VkDebugUtilsMessengerEXT m_debugMessenger;

        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_Device;
        VkQueue m_graphicQueue;
        VkQueue m_PresentQueue;
        VkSurfaceKHR m_Surface;

        static Engine* s_Instance;

        std::unique_ptr<class Window> m_Window;
    };
}