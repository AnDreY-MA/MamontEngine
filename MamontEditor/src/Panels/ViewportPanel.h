#pragma once

#include "EditorPanel.h"


namespace MamontEditor
{
    class ViewportPanel final : public EditorPanel
    {
    public:
        explicit ViewportPanel(const std::string &inName = "Viewport");

        ~ViewportPanel() = default;

        virtual void Deactivate() override;

        virtual void Init() override;

        virtual void GuiRender() override;
    };
} // namespace MamontEditor
