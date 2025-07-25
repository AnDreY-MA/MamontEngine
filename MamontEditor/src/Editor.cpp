#include "Editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace MamontEditor
{
    Editor *Editor::s_Instance = nullptr;

    Editor::Editor()
    {
        s_Instance = this;
    }

    Editor *Editor::Get()
    {
        return s_Instance;
    }

    void Editor::Init()
    {
        fmt::println("Editor init");

        AddPanel<SceneHierarchyPanel>();
        AddPanel<InspectorPanel>();
        AddPanel<StatisticsPanel>();

    }

    void Editor::Deactivate()
    {
        for (auto &[hash, panel] : m_Panels)
        {
            panel->Deactivate();
        }
        m_Panels.clear();
    }

    void Editor::ImGuiRender()
    {
        for (auto &[hash, panel] : m_Panels)
        {
            panel->GuiRender();
        }
        
    }
}