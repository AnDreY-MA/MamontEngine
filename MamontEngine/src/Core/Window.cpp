#include "Core/Window.h"

#include "Platforms/Windows/WindowsWindow.h"

namespace MamontEngine
{
    std::unique_ptr<Window> Window::Create(const WindowProps& inProps)
    {
        return std::make_unique<WindowsWindow>(inProps);
    }
}