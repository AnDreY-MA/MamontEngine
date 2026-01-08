#include "Panels/SceneSettingsPanel.h"
#include "Core/Engine.h"
#include "UI/UI.h"
#include "ECS/SceneRenderer.h"

namespace MamontEditor
{
    SceneSettingsPanel::SceneSettingsPanel(const std::string &inName) : 
        EditorPanel(inName)
    {

    }

    void SceneSettingsPanel::GuiRender()
    {
        if (OnBegin())
        {
            ImGui::SameLine();

            OnEnd();
        }
    }

}