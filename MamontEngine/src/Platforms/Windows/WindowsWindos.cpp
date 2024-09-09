#include "WindowsWindow.h"

#include "GLFW/glfw3.h"

#include <iostream>


namespace MamontEngine
{
    static uint32_t s_GLFWWindowCount = 0;

    WindowsWindow::WindowsWindow(const WindowProps& inProps)
    {
        Init(inProps);
    }

    WindowsWindow::~WindowsWindow()
    {
        std::cout << "~Window \n" << std::endl;

        Shutdown();
    }

    void WindowsWindow::Init(const WindowProps& inProps)
    {
        std::cout << "Init W \n" << std::endl;

        m_Data.Width = inProps.Width;
        m_Data.Height = inProps.Height;
        m_Data.Title = inProps.Title;

        if(s_GLFWWindowCount == 0)
        {
            glfwInit();
            //glfwSetErrorCallback(GLFWErrorCallback);
        }

        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
            m_Window = glfwCreateWindow((int)inProps.Width, (int)inProps.Height, m_Data.Title.c_str(), nullptr, nullptr);
            ++s_GLFWWindowCount;
        }
        
        glfwSetWindowUserPointer(m_Window, &m_Data);
        SetVSync(true);
    }

    void WindowsWindow::Shutdown()
    {
        std::cout << "HELLO";
        glfwDestroyWindow(m_Window);
        glfwTerminate();
    }

    void WindowsWindow::OnUpdate()
    {
        if(!glfwWindowShouldClose(m_Window))
            glfwPollEvents();
    }
}