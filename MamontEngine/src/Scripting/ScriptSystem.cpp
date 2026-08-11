#include "Scripting/ScriptSystem.h"
#include <sol/sol.hpp>
#include "Core/Log.h"
#include "ECS/Entity.h"
#include <ECS/Components/TagComponent.h>

#define REGISTER_COMPONENT(luaState, component, ptr) \
    {                                                \
        using namespace entt;                                                           \
        using namespace MamontEngine;                                                    \
        auto entityType = luaState["Entity"].get_or_create<sol::usertype<registry>>();    \
        entityType.set_function("Add" #component, ptr);                                    \
        entityType.set_function("Remove" #component, &Entity::RemoveComponent<component>);  \
        entityType.set_function("Get" #component, &Entity::GetComponent<component>);         \
    }

namespace
{
    void BindLogLua(sol::state& state)
    {
        auto log = state.create_table("Log");

        log.set_function("Info", [&](sol::this_state s, std::string_view message) { 
                MamontEngine::Log::Info("{}", message.data());
            });
        log.set_function("Warn", [&](sol::this_state s, std::string_view message) { MamontEngine::Log::Warn("{}", message.data()); });
        log.set_function("Error", [&](sol::this_state s, std::string_view message) { MamontEngine::Log::Error("{}", message.data()); });
    }

    void BindECS(sol::state& state)
    {
        using namespace MamontEngine;

        sol::usertype<MamontEngine::Entity> entityType = state.new_usertype<MamontEngine::Entity>("Entity", sol::constructors<sol::types<entt::entity>>());
        entityType.set_function("Destroy", &MamontEngine::Entity::Destroy);

        sol::usertype<TagComponent> nameComponent = state.new_usertype<TagComponent>("NameComponent");
        nameComponent["name"]                     = &TagComponent::Tag;
    }
}

namespace MamontEngine
{
    ScriptSystem::ScriptSystem()
    {
        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

        BindLogLua(*m_State);
        BindECS(*m_State);
    }

    ScriptSystem::~ScriptSystem()
    {
        delete m_State;
    }
}