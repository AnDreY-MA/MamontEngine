#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN

#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "GLFW/glfw3.h"

namespace MamontEngine
{
    class Engine 
    {
    public:
        Engine();
        virtual ~Engine();
        
        void Run();

    private:
        void Init();
        void Loop();
        void Cleanup();

        void OnEvent(Event& event);

        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);

        void CreateInstance();
        void SetupDebugMessenger();
        
        void PopulateDebugMessengerCreateInfor(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
            const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);

        static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

        bool CheckValidationLayerSupport();

        std::vector<const char*> GetRequiredExtensions();

    private:
        bool m_Running{true};

        VkInstance m_Instance;
        VkDebugUtilsMessengerEXT debugMessenger;

        //static Engine* s_Instance;

        std::unique_ptr<class Window> m_Window;
    };
}