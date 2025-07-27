#pragma once

namespace MamontEngine
{
    class ImGuiLayer
    {
    public:
        virtual ~ImGuiLayer() = default;

        virtual void Init() {};

        virtual void Begin() {};
        virtual void End() {};

        virtual void Deactivate() {};

        virtual void ImGuiRender() {};
    };
}