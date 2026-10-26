#include "SceneHierarchyPanel.h"
#include "Core/Engine.h"

#include <ECS/Components/TagComponent.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Graphics/Resources/Models/Mesh.h"

#include "UI/UI.h"
#include <ECS/Components/Hierarchy.h>

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
            const auto &registry = m_Scene->GetRegistry();

            for (auto [handle] : registry.storage<entt::entity>()->each())
            {
                if (registry.valid(handle))
                {
                    MamontEngine::Entity entity{handle, m_Scene.get()};

                    if (!entity.HasComponent<MamontEngine::Hierarchy>())
                    {
                        DrawEntityNode(entity);
                    }
                    else
                    {
                        auto &hierarchy = entity.GetComponent<MamontEngine::Hierarchy>();
                        if (hierarchy.Parent == entt::null)
                        {
                            DrawEntityNode(entity);
                        }
                    }
                    
                }
            }

            ImGui::PopStyleVar();

            if (ImGui::BeginPopup("Properties"))
            {
                if (ImGui::ButtonEx("Remove"))
                {
                    m_Scene->DestroyEntity(m_SelectedEntity);

                    m_SelectedEntity = {};
                }

                if (ImGui::ButtonEx("Add Child"))
                {
                    auto newChildEntity = m_Scene->CreateEntity("New Child");
                    newChildEntity.SetParent(m_SelectedEntity);
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

    void SceneHierarchyPanel::DrawEntityNode(MamontEngine::Entity& inEntity)
    {
        const auto &tag = inEntity.GetComponent<MamontEngine::TagComponent>().Tag;

        ImGuiTreeNodeFlags flags = (m_SelectedEntity == inEntity) ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanFullWidth;
        flags |= ImGuiTreeNodeFlags_FramePadding;

        const bool hasChildren = inEntity.HasComponent<MamontEngine::Hierarchy>() && !inEntity.GetComponent<MamontEngine::Hierarchy>().Children.empty();

        if (!hasChildren)
        {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uint64_t>(inEntity.GetID())), flags, tag.c_str());

        if (ImGui::IsItemClicked(0))
        {
            SelectEntity(inEntity);
        }

        if (ImGui::IsItemClicked(1))
        {
            ImGui::OpenPopup("Properties");
            SelectEntity(inEntity);
        }

        ImGui::TableNextColumn();

        if (opened && hasChildren)
        {
            if (inEntity.HasComponent<MamontEngine::Hierarchy>())
            {
                auto &hierarchy = inEntity.GetComponent<MamontEngine::Hierarchy>();
                for (const auto& child : hierarchy.Children)
                {
                    MamontEngine::Entity cEntity{child, m_Scene.get()};
                    DrawEntityNode(cEntity);
                }
            }
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
            DrawItemAddPrimitives("Monkies", "D:/Repos/MamontEngine/MamontEngine/assets/monkeyHD.glb", contextDevice);

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
            m_SelectedEntity = newEntity;
        }
    }
} // namespace MamontEditor