#include "ViewportPanel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Core/Engine.h"
#include "Core/ContextDevice.h"

namespace MamontEditor
{
    ViewportPanel::ViewportPanel(const std::string &inName) : EditorPanel(inName)
    {

    }

    void ViewportPanel::Deactivate()
    {
    }

    void ViewportPanel::Init()
    {

    }

    void ViewportPanel::GuiRender()
    {
        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
        //ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        auto &ContextDevice = MamontEngine::MEngine::Get().GetContextDevice();
        if (OnBegin(flags))
        {
            ImGui::Image(ContextDevice.GetCurrentFrame().ViewportDescriptor, ImGui::GetContentRegionAvail());
            //ImGui::Image(ContextDevice.Image.DrawImage.ImTextID.GetTexID(), ImGui::GetContentRegionAvail());
        }

        //ImGui::PopStyleVar(2);
        OnEnd();

    }
    bool ViewportPanel::IsHovered() const
    {
        return ImGui::IsWindowHovered();
    }
}