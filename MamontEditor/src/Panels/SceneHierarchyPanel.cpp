#include "SceneHierarchyPanel.h"
#include "Core/Engine.h"

#include <ECS/Components/TagComponent.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "Graphics/Mesh.h"

#include "UI/UI.h"

namespace MamontEditor
{
    SceneHierarchyPanel::SceneHierarchyPanel(const std::string &inName) 
        : EditorPanel(inName)
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

            /*if (ImGui::Button("ContextWindow"))
            {
                ImGui::OpenPopup("ContextWindow");
            }

            if (ImGui::BeginPopup("ContextWindow"))
            {
                DrawContextMenu();
                ImGui::EndPopup();
            }*/

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

            OnEnd();
        }
    }

    void SceneHierarchyPanel::DrawEntityNode(const MamontEngine::Entity& inEntity)
    {
        const auto &tag = inEntity.GetComponent<MamontEngine::TagComponent>().Tag;

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
            const auto &contextDevice = MEngine::Get().GetContextDevice();

            DrawItemAddPrimitives("Cube", "D:/Repos/MamontEngine/MamontEngine/assets/cube.glb", contextDevice);
            DrawItemAddPrimitives("Cone", "D:/Repos/MamontEngine/MamontEngine/assets/cone.glb", contextDevice);
            DrawItemAddPrimitives("Cylinder", "D:/Repos/MamontEngine/MamontEngine/assets/cylinder.glb", contextDevice);

            /*if (ImGui::MenuItem("Cube"))
            {
                auto cube = m_Scene->CreateEntity("Cube");
                const std::string cubeFile = "D:/Repos/MamontEngine/MamontEngine/assets/cube.glb";
                const auto                      &contextDevice = MEngine::Get().GetContextDevice();
                auto              cubeModel     = std::make_shared<MeshModel>(contextDevice, cubeFile);
                cube.AddComponent<MeshComponent>(std::move(cubeModel));
            }*/

            ImGui::EndMenu();
        }

        ImGui::EndMenu();
    }

    void SceneHierarchyPanel::DrawItemAddPrimitives(std::string_view inNameItem, std::string_view inPath, const MamontEngine::VkContextDevice &inContexDevice)
    {
        if (ImGui::MenuItem(inNameItem.data()))
        {
            auto              newEntity          = m_Scene->CreateEntity(inNameItem);
            auto              newModel     = std::make_shared<MamontEngine::MeshModel>(inContexDevice, inPath);
            newEntity.AddComponent<MamontEngine::MeshComponent>(std::move(newModel));
        }
    }
} // namespace MamontEditor
