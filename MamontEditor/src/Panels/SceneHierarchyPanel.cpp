#include "SceneHierarchyPanel.h"
#include "Core/Engine.h"

#include <ECS/Components/TagComponent.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "UI/UI.h"

namespace MamontEditor
{
    SceneHierarchyPanel::SceneHierarchyPanel(const std::string &inName) : EditorPanel(inName)
    {
        Init();
    }

    void SceneHierarchyPanel::Deactivate()
    {
        m_Scene.reset();
    }

    void SceneHierarchyPanel::Init()
    {
        // m_Scene = MamontEngine::MEngine::Get().GetScene();

        // fmt::println("SceneHierarchy inintialized, scene = {}", m_Scene != nullptr ? m_Scene.use_count() : 0);
    }

    void SceneHierarchyPanel::GuiRender()
    {
        if (m_Scene == nullptr)
        {
            m_Scene = MamontEngine::MEngine::Get().GetScene();
            if (m_Scene == nullptr)
                nullptr;
        }

        if (OnBegin(ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
            constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_BordersInner | ImGuiTableFlags_ScrollY;
            const float               lineHeight = ImGui::GetTextLineHeight();

            if (ImGui::Button("ContextWindow"))
            {
                ImGui::OpenPopup("ContextWindow");
            }

            if (ImGui::BeginPopup("ContextWindow"))
            {
                DrawContextMenu();
                ImGui::EndPopup();
            }

            /*if (ImGui::BeginPopupContextWindow("ContextWindow", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
            {
                DrawContextMenu();
                ImGui::EndPopup();
            }*/


            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
            const auto &view = m_Scene->GetRegistry().view<entt::entity>();
            view.each(
                    [&](auto ID)
                    {
                        const MamontEngine::Entity entity{ID, m_Scene.get()};
                        if (entity)
                        {
                            DrawEntityNode(entity);
                        }
                    });
            ImGui::PopStyleVar();

            if (ImGui::BeginPopup("Properties"))
            {
                if (ImGui::ButtonEx("Remove"))
                {
                    m_Scene->DestroyEntity(m_SeletctedEntity);

                    m_SeletctedEntity = {};

                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            OnEnd();
        }
    }

    void SceneHierarchyPanel::DrawEntityNode(MamontEngine::Entity inEntity)
    {
        auto &tag = inEntity.GetComponent<MamontEngine::TagComponent>().Tag;

        ImGuiTreeNodeFlags flags = (m_SeletctedEntity == inEntity) ? ImGuiTreeNodeFlags_Selected : 0;
        flags |= ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanFullWidth;
        flags |= ImGuiTreeNodeFlags_FramePadding;
        //flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        const bool opened = ImGui::TreeNodeEx(reinterpret_cast<void *>(static_cast<uint64_t>(inEntity.GetID())), flags, tag.c_str());


        if (ImGui::IsItemClicked())
        {
            m_SeletctedEntity = inEntity;
        }
        if (ImGui::IsItemClicked(1))
        {
            ImGui::OpenPopup("Properties", ImGuiPopupFlags_AnyPopupLevel);
            m_SeletctedEntity = inEntity;
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
            if (ImGui::MenuItem("Cube"))
            {
                auto cube = m_Scene->CreateEntity("Cube");
                auto file = loadGltf("D:/Repos/MamontEngine/MamontEngine/assets/cube.glb");
                cube.AddComponent<MeshComponent>(*file);
            }

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }
} // namespace MamontEditor
