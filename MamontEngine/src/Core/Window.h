#pragma once

#include <cstdint>

#include "Events/Event.h"
#include "GLFW/glfw3.h"


namespace MamontEngine
{
    struct WindowProps
    {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(const std::string& inTitle = "Mamont Engine", const uint32_t inWidth = 1600, const uint32_t inHeight = 900)
            : Title(inTitle), Width(inWidth), Height(inHeight){} 
    };

    class Window
    {
    public:
        using EventCallback = std::function<void(Event&)>;
        
        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;

        virtual GLFWwindow* GetGLFWWindow() const = 0;

        virtual void SetEventCallback(const EventCallback& callback) = 0;
        virtual void SetVSync(const bool enabled) = 0;
        virtual bool IsVSync() const = 0;

        static std::unique_ptr<Window> Create(const WindowProps& inProps = WindowProps());
    };
}