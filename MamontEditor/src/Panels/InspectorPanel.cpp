#include "Panels/InspectorPanel.h"
#include "Editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <ECS/Components/TagComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Entity.h>
#include "UI/UI.h"

#include "Utils/Loader.h"

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
    void DrawComponent(std::string_view                   inName,
                       MamontEngine::Scene* inScene,
                       entt::registry &inRegistry,
                       MamontEngine::Entity              &inEntity,
                       UIFunction                         inFunction,
                       const bool                         removable = true)
    {
        auto component = inRegistry.try_get<T>(inEntity);
        if (component)
        {
            constexpr ImGuiTreeNodeFlags TREE_FLAGS = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth |
                                                             ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 0.5f);

            const size_t id = entt::type_id<T>().hash();
            
            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void *>(id), TREE_FLAGS, "%s", inName.data());

            bool removeComponent = false;
            if (removable)
            {
                ImGui::PushID((int)id);

                const float frameHeight = ImGui::GetFrameHeight();
                ImGui::SameLine(ImGui::GetContentRegionMax().x - frameHeight * 1.2f);
                if (ImGui::Button("ComponentSetting", ImVec2{frameHeight * 1.2f, frameHeight}))
                    ImGui::OpenPopup("ComponentSetting");

                if (ImGui::BeginPopup("ComponentSetting"))
                {
                    if (ImGui::MenuItem("Remove Component"))
                        removeComponent = true;
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }

            if (open)
            {
                inFunction(*component, inEntity);
                ImGui::TreePop();
            }

            if (removeComponent)
            {
                inScene->RemoveComponent<T>(inEntity);
                //inEntity.RemoveComponent<T>(inEntity);
                //inRegistry.remove<T>(inEntity);
            }
        }
    }

    template <typename Component>
    void InspectorPanel::DrawAddComponent(entt::registry &reg, MamontEngine::Entity& inEntity, std::string_view inName)
    {
        if (ImGui::MenuItem(inName.data()))
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
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 1.75f);
     
        if (inEntity.HasComponent<TagComponent>())
        {
            auto &tagComponent = inEntity.GetComponent<TagComponent>();
            MUI::InputText("##Tag", &tagComponent.Tag, 100, ImGuiInputTextFlags_EnterReturnsTrue);

        }

        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::Text("");

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("Add Component");
        }
        if (ImGui::BeginPopup("Add Component"))
        {
            DrawAddComponent<MeshComponent>(m_SceneContext->GetRegistry(), m_Selected, "Mesh Component");

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        const auto idStr = fmt::format("UUID: {}", (uint64_t)inEntity.GetID());
        ImGui::Text(idStr.c_str());

        DrawComponent<TransformComponent>("Transform Component",
                                                        m_SceneContext,
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

        DrawComponent<MeshComponent>("Mesh Component",
                                     m_SceneContext,
                                     m_SceneContext->GetRegistry(),
                                     inEntity,
                                     [](MeshComponent &component, entt::entity entity)
                                     {
                                         // MUI::BeginProperties();
                                         if (component.Mesh)
                                         {
                                             //ImGui::Text("Nodes count: %i", component.Mesh->GetSizeNodes());
                                             ImGui::Text("Meshes count: %i", component.Mesh->GetSizeMeshes());
                                             ImGui::Text("Materials count: %i", component.Mesh->GetSizeMaterials());
                                         }
                                         
                                         
                                         // MUI::EndProperties();
                                          });

    }
    
} // namespace MamontEditor
