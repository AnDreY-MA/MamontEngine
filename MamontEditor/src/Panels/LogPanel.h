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

        virtual void Deactivate() override;

        virtual void Init() override;

        virtual void GuiRender() override;

        //void AddLog(std::string_view fmt, ...) IM_FMTARGS(2);

    private:
        /*void Clear()
        {
            m_TextBuffer.clear();
            m_LineOffsets.clear();
            m_LineOffsets.push_back(0);
        }*/

    private:
        ImGuiTextFilter m_TextFilter;
        bool            AutoScroll{true};

        bool m_IsShowInfo{true};
        bool m_IsShowWarning{true};
        bool m_IsShowError{true};
    };
} // namespace MamontEditor
