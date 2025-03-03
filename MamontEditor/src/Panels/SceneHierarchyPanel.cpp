#include "SceneHierarchyPanel.h"
#include "Core/Engine.h"

#include "imgui/imgui.h"
#include <ECS/Components/TagComponent.h>

#include "Core/Engine.h"

namespace MamontEditor
{
    SceneHierarchyPanel::SceneHierarchyPanel(std::string_view inName) : EditorPanel(inName)
    {
        
    }

    void SceneHierarchyPanel::Deactivate()
    {
        m_Scene.reset();
    }

    void SceneHierarchyPanel::Init()
    {
        m_Scene = MamontEngine::MEngine::Get().GetScene();

        fmt::println("SceneHierarchy inintialized, scene = {}", m_Scene.use_count());
    }

    void SceneHierarchyPanel::GuiRender()
    {
        if (!m_Scene)
            return;
        if (OnBegin(ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar))
        {
            auto view = m_Scene->GetRegistry().view<entt::entity>();
            view.each(
                    [&](auto ID) 
                    { 
                        MamontEngine::Entity entity{ID, m_Scene.get()};
                        DrawEntityNode(entity);
                    });

            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Empty Entity"))
                {
                    m_Scene->CreateEntity("New Entity");
                }

                ImGui::EndMenu();
            }

            OnEnd();
        }
        
    }

    void SceneHierarchyPanel::DrawEntityNode(MamontEngine::Entity inEntity)
    {
        auto &tag = inEntity.GetComponent<MamontEngine::TagComponent>().Tag;

        ImGuiTreeNodeFlags flags = ((m_SeletctedEntity == inEntity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
        const bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)m_SeletctedEntity, flags, tag.c_str());
        if (ImGui::IsItemClicked())
        {
            m_SeletctedEntity = inEntity;
        }

        if (opened)
        {
            ImGuiTreeNodeFlags flags  = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            
            ImGui::TreePop();
        }

    }
}