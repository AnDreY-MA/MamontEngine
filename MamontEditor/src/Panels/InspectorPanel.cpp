#include "Panels/InspectorPanel.h"
#include "Editor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <ECS/Components/TagComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Entity.h>
#include "UI/UI.h"
#include "Graphics/Resources/Models/Model.h"
#include "Core/Engine.h"
#include "EditorUtils/EditorUtils.h"

namespace MamontEditor
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;
    const std::string AssetsPath      = RootDirectories + "/MamontEngine/assets";

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
        m_IsHovered = ImGui::IsWindowHovered();
        OnEnd();
    }

    bool InspectorPanel::IsHovered() const
    {
        return m_IsHovered;
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
                                     [&](MeshComponent &component, entt::entity entity)
                                     {
                                         // MUI::BeginProperties();
                                         ImGui::Text("Meshes count: %i", component.Mesh->GetSizeMeshes());

                                         const std::string selectedString = component.Mesh != nullptr ? component.Mesh->GetPathFile() : "";
                                         if (ImGui::BeginCombo("##File", selectedString.c_str()))
                                         {
                                             constexpr auto                 extensionFile = ".glb";
                                             const std::vector<std::string> files         = ScanFolder(AssetsPath, extensionFile);
                                             for (size_t index = 0; index < files.size(); ++index)
                                             {
                                                 const auto       &file       = files[index];
                                                 const std::string fullPath   = AssetsPath + "/" + file;
                                                 const bool        isSelected = (component.Mesh != nullptr && component.Mesh->GetPathFile() == fullPath);

                                                 if (ImGui::Selectable(file.c_str(), isSelected))
                                                 {
                                                     auto newModel = std::make_shared<MeshModel>(MEngine::Get().GetContextDevice(), inEntity.GetID(), fullPath);
                                                     m_SceneContext->RemoveComponent<MeshComponent>(inEntity);
                                                     inEntity.AddComponent<MeshComponent>(newModel);
                                                 }

                                                 if (isSelected)
                                                 {
                                                     ImGui::SetItemDefaultFocus();
                                                 }
                                             }
                                             ImGui::EndCombo();
                                         }

                                         if (component.Mesh)
                                         {
                                             if (ImGui::TreeNodeEx("Materials"))
                                             {
                                                 for (size_t i{0}; i < component.Mesh->GetSizeMaterials(); i++)
                                                 {
                                                     auto material = component.Mesh->GetMaterial(i);
                                                     if (!material)
                                                         return;

                                                     if (ImGui::TreeNode(fmt::format("{}", material->Name).c_str()))
                                                     {
                                                         if (ImGui::ColorEdit4("Base Color", &material->Constants.ColorFactors[0]))
                                                             material->IsDity = true;
                                                         if(ImGui::DragFloat("Metallic", &material->Constants.MetalicFactor, 0.01f, 0.f, 1.f))
                                                         {
                                                             material->IsDity = true;
                                                         }
                                                         if (ImGui::DragFloat("Roughness", &material->Constants.RoughFactor, 0.01f, 0.f, 1.f))
                                                         {
                                                             material->IsDity = true;
                                                         }


                                                         ImGui::TreePop();
                                                     }
                                                 }

                                                 ImGui::TreePop();
                                             }
                                             
                                         }


                                         // MUI::EndProperties();
                                          });

    }
    
} // namespace MamontEditor
