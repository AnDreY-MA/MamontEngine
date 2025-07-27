#include "StatisticsPanel.h"
#include "Core/Engine.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace MamontEditor
{
    StatisticsPanel::StatisticsPanel(const std::string &inName) : EditorPanel(inName)
    {

    }

    void StatisticsPanel::Init()
    {
    }

    void StatisticsPanel::GuiRender()
    {
        if (OnBegin())
        {
            const MamontEngine::EngineStats &stats = MamontEngine::MEngine::Get().GetStats();
            ImGui::Text("frametime %f ms", stats.FrameTime);
            ImGui::Text("drawtime %f ms", stats.MeshDrawTime);

            OnEnd();
        }
    }
    void StatisticsPanel::Deactivate()
    {
    }
} // namespace MamontEditor
