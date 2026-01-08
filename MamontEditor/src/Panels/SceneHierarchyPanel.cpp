#include "SceneHierarchyPanel.h"
#include "Core/Engine.h"

#include <ECS/Components/TagComponent.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Graphics/Resources/Models/Mesh.h"

#include "UI/UI.h"

namespace MamontEditor
{
    SceneHierarchyPanel::SceneHierarchyPanel(const std::string &inName) 
        : EditorPanel(inName)
    {
    }

    void SceneHierarchyPanel::GuiRender()
    {
        if (m_Scene == nullptr)
        {
            m_Scene = MamontEngine::MEngine::Get().GetScene();
            return;
        }

        if (OnBegin(ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
            constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_BordersInner | ImGuiTableFlags_ScrollY;
            const float               lineHeight = ImGui::GetTextLineHeight();

            if (ImGui::BeginPopupContextWindow("ContextWindow",
                                               ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_NoOpenOverExistingPopup))
            {
                DrawContextMenu();
                ImGui::EndPopup();
            }


            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            const auto &view = m_Scene->GetRegistry().view<MamontEngine::IDComponent>();
            view.each(
                    [&](entt::entity handle, const MamontEngine::IDComponent &idComponent)
                    {
                        const MamontEngine::Entity &entity = m_Scene->GetEntity(idComponent.ID);
                        if (entity)
                        {
                            DrawEntityNode(entity);
                        }
                    });
            ImGui::PopStyleVar();

            if (ImGui::BeginPopup("Properties", ImGuiPopupFlags_MouseButtonRight))
            {
                if (ImGui::ButtonEx("Remove"))
                {
                    m_Scene->DestroyEntity(m_SeletctedEntity);

                    m_SeletctedEntity = {};

                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            m_IsHovered = ImGui::IsWindowHovered();

            OnEnd();
        }
    }

    bool SceneHierarchyPanel::IsHovered() const
    {
        return m_IsHovered;
    }

    void SceneHierarchyPanel::DrawEntityNode(const MamontEngine::Entity& inEntity)
    {
        const auto &tag = inEntity.GetComponent<MamontEngine::TagComponent>().Tag;

        ImGuiTreeNodeFlags flags = (m_SeletctedEntity == inEntity) ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanFullWidth;
        flags |= ImGuiTreeNodeFlags_FramePadding;

        const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uint64_t>(inEntity.GetID())), flags, tag.c_str());

        if (ImGui::IsItemClicked())
        {
            SelectEntity(inEntity);
        }
        if (ImGui::IsItemClicked(1))
        {
            ImGui::OpenPopup("Properties", ImGuiPopupFlags_AnyPopupLevel);
            SelectEntity(inEntity);
        }

        ImGui::TableNextColumn();

        if (opened)
        {
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::DrawContextMenu()
    {
        using namespace MamontEngine;
        if (!ImGui::BeginMenu("Create"))
            return;

        if (ImGui::MenuItem("Empty Entity"))
        {
            m_Scene->CreateEntity("New Entity");
        }

        if (ImGui::BeginMenu("Primitives"))
        {
            const auto &contextDevice = MEngine::Get().GetContextDevice();

            DrawItemAddPrimitives("Cube", "D:/Repos/MamontEngine/MamontEngine/assets/cube.glb", contextDevice);
            DrawItemAddPrimitives("Cone", "D:/Repos/MamontEngine/MamontEngine/assets/cone.glb", contextDevice);
            DrawItemAddPrimitives("Cylinder", "D:/Repos/MamontEngine/MamontEngine/assets/cylinder.glb", contextDevice);
            DrawItemAddPrimitives("Plane", "D:/Repos/MamontEngine/MamontEngine/assets/plane.glb", contextDevice);

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    void SceneHierarchyPanel::DrawItemAddPrimitives(std::string_view inNameItem, std::string_view inPath, const MamontEngine::VkContextDevice &inContexDevice)
    {
        if (ImGui::MenuItem(inNameItem.data()))
        {
            auto              newEntity          = m_Scene->CreateEntity(inNameItem);
            auto              newModel     = std::make_shared<MamontEngine::MeshModel>(newEntity.GetID(), inPath);
            newEntity.AddComponent<MamontEngine::MeshComponent>(std::move(newModel));
        }
    }
} // namespace MamontEditor