#pragma once 

#include "Panels/EditorPanel.h"
#include <ECS/Entity.h>

namespace MamontEditor
{
	class InspectorPanel : public EditorPanel
	{
    public:
        explicit InspectorPanel();
        ~InspectorPanel() = default;

        virtual void GuiRender() override;

        virtual bool IsHovered() const override;

    private:
        void DrawComponents(MamontEngine::Entity inEntity);

        template <typename Component>
        static void DrawAddComponent(entt::registry &reg, MamontEngine::Entity& inEntity, std::string_view inName);

    private:
        MamontEngine::Entity m_Selected;
        MamontEngine::Scene *m_SceneContext{nullptr};
	};

    
} // namespace MamontEditor
