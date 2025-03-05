#pragma once

#include "EditorPanel.h"
#include <memory>

#include <ECS/Entity.h>
#include <ECS/Scene.h>

namespace MamontEditor
{
    class SceneHierarchyPanel final : public EditorPanel
    {
    public:
        explicit SceneHierarchyPanel(std::string_view inName = "Scene Hierachy");

        ~SceneHierarchyPanel() = default;

        virtual void Deactivate() override;

        virtual void Init() override;

        virtual void GuiRender() override;

        MamontEngine::Entity GetSelectedEntity() const
        {
            return m_SeletctedEntity;
        }

        MamontEngine::Scene* GetSceneContext()
        {
            return m_Scene.get();
        }

    private:
        void DrawEntityNode(MamontEngine::Entity inEntity);

        void DrawContextMenu();

    private:
        std::shared_ptr<MamontEngine::Scene> m_Scene;
        MamontEngine::Entity                 m_SeletctedEntity;
    };
}