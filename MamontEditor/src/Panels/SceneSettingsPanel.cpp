#include "Panels/SceneSettingsPanel.h"
#include "Core/Engine.h"
#include "UI/UI.h"
#include "ECS/SceneRenderer.h"
#include "Core/Log.h"
#include "Core/Engine.h"
#include "EditorUtils/PropertyInspector.h"
#include "Physics/PhysicsSystem.h"

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
            if (ImGui::Button("Reload Shader"))
            {
                MamontEngine::MEngine::Get().GetRenderer()->InitPipelines();
            }

            bool isDrawBounds = MamontEngine::MEngine::Get().GetSceneRenderer()->IsDrawCollisionBounds();
            if (ImGui::Checkbox("Draw collision bounds", &isDrawBounds))
            {
                MamontEngine::MEngine::Get().GetSceneRenderer()->EnableDrawCollisionBounds(isDrawBounds);
            }

            if (ImGui::CollapsingHeader("Physics System"))
            {
                //auto physicsSystem = MamontEngine::MEngine::Get().GetPhysicsSytem();
                //auto s             = entt::make_any<HeroPhysics::PhysicsSystem>(physicsSystem);
               // InspectComponent({}, physicsSystem);
            }


            OnEnd();
        }
    }

}