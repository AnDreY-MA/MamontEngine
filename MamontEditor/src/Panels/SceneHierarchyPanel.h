#pragma once

#include "EditorPanel.h"
#include <memory>

#include <ECS/Entity.h>
#include <ECS/Scene.h>

namespace MamontEditor
{
    struct MamontEngine::VkContextDevice;

    class SceneHierarchyPanel final : public EditorPanel
    {
    public:
        explicit SceneHierarchyPanel(const std::string &inName = "Scene Hierachy");

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
            return m_Scene != nullptr ? m_Scene.get() : nullptr;
        }

    private:
        void DrawEntityNode(const MamontEngine::Entity& inEntity);

        void DrawContextMenu();

        void DrawItemAddPrimitives(std::string_view inNameItem, std::string_view inPath, const MamontEngine::VkContextDevice &inContexDevice);

    private:
        std::shared_ptr<MamontEngine::Scene> m_Scene;
        MamontEngine::Entity                 m_SeletctedEntity;
    };
}