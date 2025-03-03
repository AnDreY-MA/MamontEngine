#include "Editor.h"

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

        m_SceneHierarchy = std::make_unique<SceneHierarchyPanel>("Scene");
        m_SceneHierarchy->Init();
        m_Inspector = std::make_unique<InspectorPanel>();
        m_Inspector->Init();
    }

    void Editor::Deactivate()
    {
        m_SceneHierarchy->Deactivate();
        m_Inspector->Deactivate();
    }

    void Editor::ImGuiRender()
    {
        m_SceneHierarchy->GuiRender();
        m_Inspector->GuiRender();
    }
}