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
            const auto                      &engine{MamontEngine::MEngine::Get()};

            const MamontEngine::RenderStats &stats = engine.GetStats();
            //ImGui::Text("frametime %f ms", stats.FrameTime);
            ImGui::Text("Framerate: %f ms", io.Framerate);
            ImGui::Text("Drawtime: %f ms", stats.MeshDrawTime);

            /*const auto &memoryStats = engine.GetGPUMemoryStats();
            ImGui::Text("MemoryUsed: %f", memoryStats.MemoryUsed);
            ImGui::Text("MemoryAllocated: %f", memoryStats.MemoryAllocated);*/


            m_IsHovered = ImGui::IsWindowHovered();

            if (ImGui::BeginChild("Lighting"))
            {
                const auto &lightData = engine.GetSceneRenderer()->GetLightData();
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
