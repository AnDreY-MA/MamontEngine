#include "Panels/EditorPanel.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace MamontEditor
{
    EditorPanel::EditorPanel(const std::string &inName) : 
        m_Name(inName)
    {

    }
    bool EditorPanel::OnBegin(int32_t inWindowFlags) 
    {
        if (!isVisible)
            return false;

        //ImGui::SetNextWindowSize(ImVec2(480, 640), ImGuiCond_Once);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        
        ImGui::Begin(m_Name.c_str(), &isVisible, inWindowFlags | ImGuiWindowFlags_NoCollapse);
        return true;
    }

    void EditorPanel::OnEnd() const
    {
        ImGui::PopStyleVar();
        ImGui::End();
    }
}