#pragma once 

#include "Panels/EditorPanel.h"
#include <ECS/Entity.h>

namespace MamontEditor
{
	class InspectorPanel : public EditorPanel
	{
    public:
        explicit InspectorPanel();
        ~InspectorPanel();

        virtual void GuiRender() override;

        virtual bool IsHovered() const override;

    private:
        void DrawComponents(MamontEngine::Entity inEntity);

        template <typename Component>
        static void DrawAddComponent(entt::registry &reg, MamontEngine::Entity& inEntity, std::string_view inName);

    private:
        MamontEngine::Entity m_Selected;
        std::shared_ptr<MamontEngine::Scene> m_SceneContext;
	};

    
} // namespace MamontEditor
