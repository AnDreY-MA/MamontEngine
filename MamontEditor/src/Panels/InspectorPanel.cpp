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
#include "EditorUtils/PropertyInspector.h"
#include <ECS/Components/DirectionLightComponent.h>
#include <ECS/Components/PointLightComponent.h>
#include <ECS/Components/RigidbodyComponent.h>
#include <ECS/Components/Component.h>
#include <ECS/Components/ScriptComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/ComponentRegistry.h>
#include <entt/core/hashed_string.hpp>
#include "Math/Color.h"
#include "Physics/Collision/BoxCollision.h"
#include "Physics/Collision/SphereCollision.h"
#include "Utils/MetaInspector.h"

const std::string RootDirectories = PROJECT_ROOT_DIR;
const std::string AssetsPath      = RootDirectories + "/MamontEngine/assets";

namespace MamontEditor
{
    InspectorPanel::InspectorPanel() : 
        EditorPanel("Inspector")
    {

    }
    
    InspectorPanel::~InspectorPanel()
    {
        m_SceneContext.reset();
    }

    void InspectorPanel::GuiRender()
    {
        m_Selected = Editor::Get()->GetSelectedEntity();
        m_SceneContext = Editor::Get()->GetSceneContext();

        if (!OnBegin())
            return;

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

    void InspectorPanel::DrawComponents(MamontEngine::Entity entity)
    {
        using namespace MamontEngine;

        ImGui::PushItemWidth(-1.0f);

        if (entity.HasComponent<TagComponent>())
        {
            auto &tagComponent = entity.GetComponent<TagComponent>();
            MUI::InputText("##Tag", &tagComponent.Tag, 100, ImGuiInputTextFlags_EnterReturnsTrue);
        }

        ImGui::PopItemWidth();

        if (ImGui::Button("Add Component"))
        {
            ImGui::OpenPopup("Add Component");
        }
        if (ImGui::BeginPopup("Add Component"))
        {
            /* DrawAddComponent<MeshComponent>(m_SceneContext->GetRegistry(), m_Selected, "Mesh Component");
            DrawAddComponent<DirectionLightComponent>(m_SceneContext->GetRegistry(), m_Selected, "Direction Light Component");
            DrawAddComponent<PointLightComponent>(m_SceneContext->GetRegistry(), m_Selected, "Point Light Component");
            DrawAddComponent<RigidbodyComponent>(m_SceneContext->GetRegistry(), m_Selected, "Rigidbody Component");
            DrawAddComponent<HeroPhysics::BoxCollision>(m_SceneContext->GetRegistry(), m_Selected, "BoxCollision Component");
            DrawAddComponent<HeroPhysics::SphereCollision>(m_SceneContext->GetRegistry(), m_Selected, "SphereCollision Component");
            DrawAddComponent<ScriptComponent>(m_SceneContext->GetRegistry(), m_Selected, "Script Component");*/
            //DrawAddComponent<CameraComponent>(m_SceneContext->GetRegistry(), m_Selected, "Camera Component");

            for (const auto& entry : ComponentRegistry::Instance().GetEntries())
            {
                if (!entry.Has(m_SceneContext->GetRegistry(), m_Selected))
                {
                    if (ImGui::MenuItem(entry.Name.data()))
                    {
                        entry.Emplace(m_SceneContext->GetRegistry(), m_Selected);
                        ImGui::CloseCurrentPopup();
                    }
                }
            }

            ImGui::EndPopup();
        }

        auto &registry = m_SceneContext->GetRegistry();
        for (auto &&[id, storage] : registry.storage())
        {
            if (!storage.contains(m_Selected))
                continue;

            auto componentMeta = entt::resolve(storage.type());
            if (!componentMeta)
                continue;

            entt::meta_any metaProp = componentMeta.from_void(storage.value(m_Selected));

            InspectComponent(m_Selected, metaProp);
        }

        ImGui::SameLine();

        if (entity.HasComponent<ScriptComponent>())
        {
            constexpr ImGuiTreeNodeFlags TREE_FLAGS = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                                      ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

            const bool menuOpen = ImGui::CollapsingHeader("ScriptComponent", TREE_FLAGS);
            if (!menuOpen)
                return;

            const std::string id = "ScriptComponent";
            ImGui::PushID(id.c_str());
            ImGui::Indent(10.f);

            auto &scriptComponent = entity.GetComponent<ScriptComponent>();

            const std::string selectedString = scriptComponent.GetFilePath();
            if (ImGui::BeginCombo("##File", selectedString.c_str()))
            {
                constexpr auto                 extensionFile = ".lua";
                const std::vector<std::string> files         = MamontEditor::ScanFolder(AssetsPath, extensionFile, true);
                for (const auto &file : files)
                {
                    const std::string fullPath   = AssetsPath + "/Scripts/" + file;
                    const bool        isSelected = (selectedString == fullPath);

                    if (ImGui::Selectable(file.c_str(), isSelected))
                    {
                        scriptComponent.LoadScript(fullPath.c_str());
                    }

                    if (isSelected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }   

            ImGui::Unindent(10);
            ImGui::Separator();

            ImGui::PopID();
        }
    }
    
} // namespace MamontEditor