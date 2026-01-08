#pragma once

#include "EditorPanel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace MamontEditor
{
    class LogPanel final : public EditorPanel
    {
    public:
        explicit LogPanel(const std::string &inName = "Log");

        ~LogPanel() = default;

        virtual void GuiRender() override;

        virtual bool IsHovered() const override;

    private:
        ImGuiTextFilter m_TextFilter;
        bool            AutoScroll{true};

        bool m_IsShowInfo{true};
        bool m_IsShowWarning{true};
        bool m_IsShowError{true};
    };
} // namespace MamontEditor
