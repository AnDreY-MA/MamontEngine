#include "Panels/InspectorPanel.h"
#include "Editor.h"
#include "imgui/imgui.h"
#include <ECS/Components/TagComponent.h>
#include <ECS/Components/TransformComponent.h>

#include "UI/UI.h"

namespace MamontEditor
{
    InspectorPanel::InspectorPanel() : 
        EditorPanel("Inspector")
    {
        
    }

    void InspectorPanel::GuiRender()
    {
        m_Selected = Editor::Get()->GetSelectedEntity();
        m_SceneContext = Editor::Get()->GetSceneContext();

        OnBegin();
        if (m_Selected)
        {
            DrawComponents(m_Selected);
        }

        OnEnd();
    }

    template<typename T, typename UIFunction>
    void DrawComponent(std::string_view inName, entt::registry& inRegistry, MamontEngine::Entity inEntity, UIFunction inFunction, const bool removable = true)
    {
        auto component = inRegistry.try_get<T>(inEntity);
        if (component)
        {
            const ImGuiTreeNodeFlags TREE_FLAGS = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth |
                                                             ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 0.5f);

            const size_t id = entt::type_id<T>().hash();
            
            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void *>(id), TREE_FLAGS, "%s", inName);

            bool removeComponent = false;
            if (removable)
            {

            }

            if (open)
            {
                inFunction(*component, inEntity);
                ImGui::TreePop();
            }
            if (removeComponent)
            {
                inRegistry.remove<T>(inEntity);
            }
        }
    }

    template <typename Component>
    void InspectorPanel::draw_add_component(entt::registry &reg, MamontEngine::Entity inEntity, std::string_view inName)
    {
        if (ImGui::MenuItem(inName))
        {
            if (!reg.all_of<Component>(inEntity))
            {
                reg.emplace<Component>(inEntity);
            }
            else
            {
                fmt::print("Entity already has");
            }

            ImGui::CloseCurrentPopup();
        }
    }

    void InspectorPanel::DrawComponents(MamontEngine::Entity inEntity)
    {
        using namespace MamontEngine;
        TagComponent *tagComponent = m_SceneContext->GetRegistry().try_get<TagComponent>(inEntity);
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * .75f);
        if (tagComponent)
        {
            MUI::InputText("##Tag", &tagComponent->Tag, 256, ImGuiInputTextFlags_EnterReturnsTrue);
        }

        ImGui::PopItemWidth();
        ImGui::SameLine();


        ImGui::SameLine();


        DrawComponent<TransformComponent>(" Transform Component",
                                                        m_SceneContext->GetRegistry(),
                                                        inEntity,
                                                        [](MamontEngine::TransformComponent &component, entt::entity entity) 
            {
                //MUI::BeginProperties();
                MUI::DrawVec3Control("Translation", component.Translation);
                auto rotation = glm::degrees(component.Rotation);
                MUI::DrawVec3Control("Rotation", rotation);
                component.Rotation = glm::radians(rotation);
                MUI::DrawVec3Control("Scale", component.Scale, 1.f);
                //MUI::EndProperties();
            });

    }
    
} // namespace MamontEditor
