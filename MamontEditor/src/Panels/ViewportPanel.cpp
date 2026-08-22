#include "ViewportPanel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Core/Engine.h"
#include "Core/ContextDevice.h"
#include <Graphics/Devices/LogicalDevice.h>
#include <ImGuizmo.h>

namespace MamontEditor
{
    ViewportPanel::ViewportPanel(const std::string &inName) 
        : EditorPanel(inName)
    {
        
    }

    void ViewportPanel::GuiRender()
    {
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;

        bool hovered = false;

        auto &ContextDevice = MamontEngine::MEngine::Get().GetContextDevice();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        if (OnBegin(flags))
        {
            if (ImGui::BeginChild("##ChildViewport"))
            {
                ImGui::Image(ContextDevice.GetCurrentFrame().ViewportDescriptor, ImGui::GetContentRegionAvail());
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
                hovered = ImGui::IsWindowHovered() && !ImGuizmo::IsUsing();
            }
            
            ImGui::EndChild();
        }

        ImGui::PopStyleVar(2);
        OnEnd();

    }
    bool ViewportPanel::IsHovered() const
    {
        return ImGui::IsWindowHovered();
    }
}