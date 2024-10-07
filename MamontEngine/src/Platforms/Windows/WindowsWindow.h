#pragma once

#include <cstdint>
#include "Core/Window.h"

namespace MamontEngine
{
    class WindowsWindow : public Window
    {
    public:
        WindowsWindow(const WindowProps& inProps);

        virtual ~WindowsWindow();

        virtual uint32_t GetWidth() const override { return m_Data.Width; }
        virtual uint32_t GetHeight() const override { return m_Data.Height; }

        virtual GLFWwindow *GetGLFWWindow() const
        {
            return m_Window;
        }
        
        virtual void SetEventCallback(const EventCallback& callback) override { m_Data.Callback = callback;}
        virtual void SetVSync(const bool enabled) override { m_Data.VSync = enabled;}
        virtual bool IsVSync() const override { return m_Data.VSync;}

        virtual void OnUpdate() override;
        
    private:
        void Init(const WindowProps& inProps);
        void Shutdown();

    private:
        GLFWwindow* m_Window;

        struct WindowData
        {
            std::string Title;
            unsigned int Width, Height;
            bool VSync;

            EventCallback Callback;
        };

        WindowData m_Data;
    };
}