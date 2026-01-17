#pragma once

namespace MamontEngine
{
    class ImGuiLayer : NonMovable
    {
    public:
        virtual ~ImGuiLayer() = default;

        virtual void Init() {};

        virtual void Deactivate() {};

        virtual void ImGuiRender() = 0;

    protected:
        virtual void Begin() = 0;
        virtual void End() = 0;
    };
}