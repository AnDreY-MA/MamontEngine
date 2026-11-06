#pragma once

#include "Component.h"
#include <sol/forward.hpp>
#include "ECS/Entity.h"

namespace MamontEngine
{
    class Script;

	struct ScriptComponent : public Component
	{
        REFLECT()
    public:
        explicit ScriptComponent();
        explicit ScriptComponent(std::string_view inFile);
        ~ScriptComponent();

        void BeginPlay(Entity inOwner);
        void Update(float inDeltaTime);
        void EndPlay();

        void                      Reload();

        Entity GetOwner() const { return m_EntityOwner; }

	private:
        std::shared_ptr<Script>                  m_Script;
        std::shared_ptr<sol::protected_function> m_OnBeginFunc;
        std::shared_ptr<sol::protected_function> m_OnUpdateFunc;
        std::shared_ptr<sol::protected_function> m_OnEndFunc;

        Entity m_EntityOwner;

        template <class Archive>
        friend void save(Archive &ar, const ScriptComponent &scirpt)
        {
        }
        template <class Archive>
        friend void load(Archive &ar, ScriptComponent &scirpt)
        {

        }
	};
}