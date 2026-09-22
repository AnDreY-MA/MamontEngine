#include "Scripting/ScriptSystem.h"
#include <sol/sol.hpp>
#include "Core/Log.h"
#include "ECS/Entity.h"
#include <ECS/Components/TagComponent.h>
#include <ECS/Components/ScriptComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>

#define REGISTER_COMPONENT_ECS(luaState, component) \
    {                                                \
        using namespace entt;                                                           \
        using namespace MamontEngine;                                                    \
        auto entityType = luaState["Entity"].get_or_create<sol::usertype<MamontEngine::Entity>>();    \
        entityType.set_function("Add" #component, &Entity::AddComponent<component>);   \
        entityType.set_function("Remove" #component, &Entity::RemoveComponent<component>);   \
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

    template <typename T, typename... Args>
    sol::usertype<T> BindMathStruct(sol::state &state, std::string_view name, Args &&...args)
    {
        sol::usertype<T> struct_type = state.new_usertype<glm::vec3>("Vector3", std::forward<Args>(args)...);

        /*vector3_type["x"] = &glm::vec3::x;
        vector3_type["y"] = &glm::vec3::y;
        vector3_type["z"] = &glm::vec3::z;*/

        struct_type[sol::meta_function::addition] = [](const T &a, const T &b) { return a + b; };

        struct_type[sol::meta_function::subtraction] = [](const T &a, const T &b) { return a - b; };

        const auto multiply_overloads = sol::overload([](const T &v1, const T &v2) -> T { return v1 * v2; },
                                                      [](const T &v1, const float f) -> T { return v1 * f; },
                                                      [](const float f, const T &v1) -> T { return f * v1; });

        struct_type[sol::meta_function::multiplication] = multiply_overloads;

        struct_type[sol::meta_function::division] = [](const T &a, const T &b) { return a / b; };

        struct_type[sol::meta_function::equal_to] = [](const T &a, const T &b) { return a == b; };

        return struct_type;
    }

    void BindMathsLua(sol::state& state)
    {
        auto vector3 = BindMathStruct<glm::vec3>(state, "Vector3", sol::constructors<glm::vec3(), glm::vec3(float, float, float)>());
        vector3["x"]      = &glm::vec3::x;
        vector3["y"]      = &glm::vec3::y;
        vector3["z"]      = &glm::vec3::z;
    }

    void BindECS(sol::state& state)
    {
        using namespace MamontEngine;

        sol::usertype<entt::registry> enttRegistry = state.new_usertype<entt::registry>("EnttRegistry");

        sol::usertype<MamontEngine::Entity> entityType = state.new_usertype<MamontEngine::Entity>("Entity", sol::constructors<sol::types<entt::entity>>());
        entityType.set_function("Destroy", &MamontEngine::Entity::Destroy);

        sol::usertype<TagComponent> nameComponent_type = state.new_usertype<TagComponent>("NameComponent");
        nameComponent_type["name"]                     = &TagComponent::Tag;
        REGISTER_COMPONENT_ECS(state, TagComponent);

        sol::usertype<ScriptComponent> scriptComponent_type =
                state.new_usertype<ScriptComponent>("ScriptComponent", sol::constructors<sol::types<std::string_view>>());
        scriptComponent_type.set_function("GetOwner", &ScriptComponent::GetOwner);
        REGISTER_COMPONENT_ECS(state, ScriptComponent);

        sol::usertype<TransformComponent> transformComponent_type = state.new_usertype<TransformComponent>("TransformComponent");
        transformComponent_type.set_function("SetPosition", &TransformComponent::SetPosition);
        transformComponent_type.set_function("GetPosition", &TransformComponent::GetPosition);
        REGISTER_COMPONENT_ECS(state, TransformComponent);

        sol::usertype<MeshComponent> meshComponent_type = state.new_usertype<MeshComponent>("MeshComponent");
        REGISTER_COMPONENT_ECS(state, MeshComponent);

    }
}

namespace MamontEngine
{
    ScriptSystem::ScriptSystem()
    {
        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

        BindLogLua(*m_State);
        BindMathsLua(*m_State);
        BindECS(*m_State);
    }

    ScriptSystem::~ScriptSystem()
    {
        delete m_State;
    }
}