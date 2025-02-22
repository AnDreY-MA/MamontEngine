#pragma once 

#include "Graphics/Renderer.h"

#include "Core/ContextDevice.h"


namespace MamontEngine
{
    class RendererVulkan : public Renderer
    {
    public:
        void Init(void *inConfiguration);
        void Shutdown();

    private:

    };
}