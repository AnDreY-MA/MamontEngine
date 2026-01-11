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
#include <ECS/Components/DirectionLightComponent.h>
#include <ECS/Components/Component.h>
#include <entt/core/hashed_string.hpp>
#include "Math/Color.h"

const std::string RootDirectories = PROJECT_ROOT_DIR;
const std::string AssetsPath      = RootDirectories + "/MamontEngine/assets";

namespace
{
    void InspectProperty(entt::registry &reg, entt::meta_any &property, entt::meta_data &propertyMeta, const entt::id_type propertyID);

    void InspectComponent(entt::registry& registry, entt::entity owner, entt::meta_any& data)
    {
        using namespace MamontEngine;
        if (!data)
        {
            Log::Warn("InspectComponent: invalid data");
            return;
        }

        const auto dataType = data.type();

        constexpr ImGuiTreeNodeFlags TREE_FLAGS = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap |
                                                  ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

        const bool menuOpen = ImGui::CollapsingHeader(dataType.name(), TREE_FLAGS);
        if (!menuOpen)
            return;

        const std::string id = std::to_string(dataType.id()).append("Component");
        ImGui::PushID(id.c_str());
        ImGui::Indent(10.f);

        for (auto &&[proprtyId, propertyMetaData] : data.type().data())
        {
            auto instance = propertyMetaData.get(data);
            
            InspectProperty(registry, instance, propertyMetaData, proprtyId);
            propertyMetaData.set(data, instance);
        }

        ImGui::Unindent(10);
        ImGui::PopID();
    }

    void InspectProperty(entt::registry &reg, entt::meta_any &property, entt::meta_data &propertyMeta, const entt::id_type propertyID)
    {
        using namespace MamontEngine;
        const auto type = propertyMeta.type();

        const char *label = propertyMeta.name() ? propertyMeta.name() : std::string{propertyMeta.type().info().name()}.data();

        ImGui::PushID((std::string(label) + "prop").c_str());

        ImGui::Indent(10);

        if (type.is_class())
        {
            if (type.info() == entt::type_id<Transform>())
            {
                Transform* transform = property.try_cast<Transform>();

                MUI::DrawVec3Control("Position", transform->Position);
                MUI::DrawVec3Control("Rotation", transform->Rotation);
                MUI::DrawVec3Control("Scale", transform->Scale, 1.f);
            }

            if (type.info() == entt::type_id<Color>())
            {
                if (auto color = property.try_cast<Color>())
                {
                    ImGui::ColorEdit4(label, color->Data());
                }
            }
        }

        if (type.info() == entt::type_id<float>())
        {
            // ImGui::Text(type.info().name().data());
            auto value = property.try_cast<float>();
            if (value)
            {
                ImGui::DragFloat("Value", value, 0.01f, 0.f);
            }
        }

        if (type.is_pointer_like())
        {
            if (entt::meta_any ref = *property; ref)
            {
                auto asset = ref.try_cast<Asset>();

                const std::string selectedString = asset != nullptr ? asset->GetPathFile() : "empty";
                if (ImGui::BeginCombo("##File", selectedString.c_str()))
                {
                    constexpr auto                 extensionFile = ".glb";
                    const std::vector<std::string> files         = MamontEditor::ScanFolder(AssetsPath, extensionFile);
                    for (const auto &file : files)
                    {
                        const std::string fullPath   = AssetsPath + "/" + file;
                        const bool        isSelected = (selectedString == fullPath);

                        if (ImGui::Selectable(file.c_str(), isSelected))
                        {
                            asset->Load(fullPath);
                        }

                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }   
                
                if (asset)
                {
                    if (auto model = static_cast<MeshModel *>(asset); model)
                    {
                        if (ImGui::TreeNodeEx("Materials"))
                        {
                            for (size_t i{0}; i < model->GetSizeMaterials(); i++)
                            {
                                auto material = model->GetMaterial(i);
                                if (!material)
                                    return;

                                if (ImGui::TreeNode(fmt::format("{}", material->Name).c_str()))
                                {
                                    if (ImGui::ColorEdit4("Base Color", &material->Constants.ColorFactors[0]))
                                        material->IsDity = true;
                                    if (ImGui::DragFloat("Metallic", &material->Constants.MetalicFactor, 0.01f, 0.f, 1.f))
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
                }
                
            }
        }

        ImGui::Unindent(10);
        ImGui::Separator();

        ImGui::PopID();
    }
}

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

        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 1.75f);

        if (entity.HasComponent<TagComponent>())
        {
            auto &tagComponent = entity.GetComponent<TagComponent>();
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
            DrawAddComponent<DirectionLightComponent>(m_SceneContext->GetRegistry(), m_Selected, "Direction Light Component");

            ImGui::EndPopup();
        }

        ImGui::SameLine();

        auto &registry = m_SceneContext->GetRegistry();
        for (auto &&[id, storage] : registry.storage())
        {
            if (!storage.contains(m_Selected))
                continue;

            auto componentMeta = entt::resolve(storage.type());
            if (!componentMeta)
                continue;

            entt::meta_any metaProp = componentMeta.from_void(storage.value(m_Selected));

            InspectComponent(registry, m_Selected, metaProp);
        }
    }
    
} // namespace MamontEditor