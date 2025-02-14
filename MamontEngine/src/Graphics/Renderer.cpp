#include "Renderer.h"
#include "Graphics/Vulkan/RendererVulkan.h"

namespace MamontEngine
{
    Renderer::API             Renderer::s_API = Renderer::API::Vulkan;

    std::unique_ptr<Renderer> Renderer::CreateRenderer()
    {
        switch (s_API)
        {
            case Renderer::API::Vulkan: return std::unique_ptr<RendererVulkan>();
        }
        return std::unique_ptr<Renderer>();
    }
} // namespace MamontEngine
