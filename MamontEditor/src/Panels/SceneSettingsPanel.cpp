#include "Panels/SceneSettingsPanel.h"
#include "Core/Engine.h"
#include "UI/UI.h"

namespace MamontEditor
{
    SceneSettingsPanel::SceneSettingsPanel(const std::string &inName) : 
        EditorPanel(inName)
    {

    }

    void SceneSettingsPanel::Init()
    {
    }

    void SceneSettingsPanel::Deactivate()
    {
        m_SceneRenderer.reset();
    }

    void SceneSettingsPanel::GuiRender()
    {
        if (OnBegin())
        {
            ImGui::SameLine();
            if (!m_SceneRenderer)
            {
                fmt::println("SceneRenderer is null");
                m_SceneRenderer = MamontEngine::MEngine::Get().GetScene()->GetSceneRenderer();

            }
            glm::vec3& LightPosition = m_SceneRenderer->CascadeLightPosition();
            MamontEngine::MUI::DrawVec3Control("Ligth Position", LightPosition);

            OnEnd();
        }
    }

}