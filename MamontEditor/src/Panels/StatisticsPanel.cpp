#include "StatisticsPanel.h"
#include "Core/Engine.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace MamontEditor
{
    StatisticsPanel::StatisticsPanel(const std::string &inName) : EditorPanel(inName)
    {

    }

    void StatisticsPanel::GuiRender()
    {
        if (OnBegin())
        {
            const ImGuiIO& io = ImGui::GetIO();
            const MamontEngine::RenderStats &stats = MamontEngine::MEngine::Get().GetStats();
            //ImGui::Text("frametime %f ms", stats.FrameTime);
            ImGui::Text("Framerate: %f ms", io.Framerate);
            ImGui::Text("Drawtime: %f ms", stats.MeshDrawTime);

            m_IsHovered = ImGui::IsWindowHovered();

            if (ImGui::BeginChild("Lighting"))
            {
                const auto &lightData = MamontEngine::MEngine::Get().GetSceneRenderer()->GetLightData();
                ImGui::Text("Directional Light: %d", lightData.HasDirectionLight);
                ImGui::Text("Point Light count: %i", lightData.PointLightingCount);

            }
            ImGui::EndChild();

            OnEnd();
        }
    }

    bool StatisticsPanel::IsHovered() const
    {
        return m_IsHovered;
    }
} // namespace MamontEditor
