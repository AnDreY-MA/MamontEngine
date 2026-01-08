#pragma once 

#include "Panels/EditorPanel.h"
#include <ECS/Entity.h>
//#include <rttr/type.h>

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
        void DrawComponents(MamontEngine::Entity entity);

        template <typename Component>
        static void DrawAddComponent(entt::registry &reg, MamontEngine::Entity& inEntity, std::string_view inName);

    private:
        MamontEngine::Entity m_Selected;
        std::shared_ptr<MamontEngine::Scene> m_SceneContext;

        //std::unordered_map<EPropertyType, std::function<void(const std::string &, rttr::variant &)>> m_PropertyDraws;
	};

    
} // namespace MamontEditor
