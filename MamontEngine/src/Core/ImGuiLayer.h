#pragma once

namespace MamontEngine
{
    class ImGuiLayer
    {
    public:
        virtual ~ImGuiLayer() = default;

        virtual void Init() {};

        virtual void Deactivate() {};

        virtual void ImGuiRender() {};
    };
}