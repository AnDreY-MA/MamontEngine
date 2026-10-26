#include "Scripting/ScriptSystem.h"
#include <sol/sol.hpp>
#include "Core/Log.h"
#include "ECS/Entity.h"
#include <ECS/Components/TagComponent.h>
#include <ECS/Components/ScriptComponent.h>
#include <ECS/Components/MeshComponent.h>
#include <ECS/Components/TransformComponent.h>
#include <ECS/Components/CameraComponent.h>
#include <ECS/Components/AudioComponent.h>
#include "Core/InputSystem.h"

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

    MamontEngine::Entity GetEntityByName(MamontEngine::Scene *scene, const std::string &name)
    {
        using namespace MamontEngine;
        entt::entity resutlEntity = entt::null;
        auto        &registry = scene->GetRegistry();

        registry.view<TagComponent>().each(
                [&](const auto &entity, const TagComponent &component)
                {
                    if (name == component.Tag)
                    {
                        resutlEntity = entity;
                    }
                });

        if (resutlEntity == entt::null)
        {
            Log::Warn("Failed to find entity: {}", name.c_str());
        }

        return {resutlEntity, scene};
    }

    void BindECS(sol::state& state)
    {
        using namespace MamontEngine;

        state.set_function("GetEntityByName", &GetEntityByName);

        sol::usertype<entt::registry> enttRegistry = state.new_usertype<entt::registry>("EnttRegistry");

        sol::usertype<MamontEngine::Entity> entityType = state.new_usertype<MamontEngine::Entity>("Entity", sol::constructors<sol::types<entt::entity>>());
        entityType.set_function("Destroy", &MamontEngine::Entity::Destroy);
        entityType.set_function("GetChildren", &MamontEngine::Entity::GetChildrenTemp);

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

        sol::usertype<CameraComponent> cameraComponent_type = state.new_usertype<CameraComponent>("CameraComponent");
        cameraComponent_type.set_function("GetFOV", &CameraComponent::GetFOV);
        cameraComponent_type.set_function("GetNear", &CameraComponent::GetNear);
        cameraComponent_type.set_function("GetFar", &CameraComponent::GetFar);

        REGISTER_COMPONENT_ECS(state, CameraComponent);
        
        sol::usertype<Scene> scene_type = state.new_usertype<Scene>("Scene");
        //scene_type.set_function("GetRigstry", &Scene::GetRegistry);

        sol::usertype<AudioComponent> audioComponent_type = state.new_usertype<AudioComponent>("AudioComponent");
        audioComponent_type.set_function("Play", &AudioComponent::Play);
        audioComponent_type.set_function("Pause", &AudioComponent::Pause);
        audioComponent_type.set_function("Stop", &AudioComponent::Stop);
        REGISTER_COMPONENT_ECS(state, AudioComponent);

    }

    void BindInput(sol::state& state)
    {
        using namespace MamontEngine;

        auto input = state["Input"].get_or_create<sol::table>();

        input.set_function("IsKeyPressed", 
            [](KeyCode key) -> bool { 
                return InputSystem::Get().IsKeyPressed(key);
            }
        );
        input.set_function("IsKeyHeld", [](KeyCode key) -> bool { 
            return InputSystem::Get().IsKeyPressed(key); 
        });

        std::initializer_list<std::pair<sol::string_view, KeyCode>> keyItems = {
            { "A", KeyCode::a },
            { "B", KeyCode::b },
            { "C", KeyCode::c },
            { "D", KeyCode::d },
            { "E", KeyCode::e },
            { "F", KeyCode::f },
            { "H", KeyCode::g },
            { "G", KeyCode::h },
            { "I", KeyCode::i },
            { "J", KeyCode::j },
            { "K", KeyCode::k },
            { "L", KeyCode::l },
            { "M", KeyCode::m },
            { "N", KeyCode::n },
            { "O", KeyCode::o },
            { "P", KeyCode::p },
            { "Q", KeyCode::q },
            { "R", KeyCode::r },
            { "S", KeyCode::s },
            { "T", KeyCode::t },
            { "U", KeyCode::u },
            { "V", KeyCode::v },
            { "W", KeyCode::w },
            { "X", KeyCode::x },
            { "Y", KeyCode::y },
            { "Z", KeyCode::z },
            //{ "UNKOWN", KeyCode::Unknown },
            { "Space", KeyCode::space },
            { "Escape", KeyCode::exclaim },
            { "Comma", KeyCode::comma },
            { "MINUS", KeyCode::minus },
            { "PERIOD", KeyCode::period },
            { "SLASH", KeyCode::slash },
            { "SEMICOLON", KeyCode::semicolon},
            { "EQUAL", KeyCode::equals },
            { "LEFT_BRACKET", KeyCode::left_bracket},
            { "BACKSLASH", KeyCode::backslash },
            { "RIGHT_BRACKET", KeyCode::right_bracket },
            //{ "BACK_TICK", KeyCode::BackTick },
            //{ "Enter", KeyCode::en },
            { "Tab", KeyCode::tab },
            { "Backspace", KeyCode::backspace },
            //{ "Insert", KeyCode::Insert },
            //{ "Delete", KeyCode::Delete },
            { "Right", KeyCode::right_paren },
            { "Left", KeyCode::left_paren },
            //{ "Down", KeyCode::Down },
            //{ "Up", KeyCode::Up },
            //{ "PageUp", KeyCode::PageUp },
            //{ "PageDown", KeyCode::PageDown },
            //{ "Home", KeyCode::home },
            //{ "End", KeyCode::End },
            { "CAPS_LOCK", KeyCode::capslock },
            //{ "SCROLL_LOCK", KeyCode::ScrollLock },
            //{ "NumLock", KeyCode::NumLock },
            //{ "PrintScreen", KeyCode::PrintScreen },
            //{ "Pasue", KeyCode::Pause },
            //{ "LeftShift", KeyCode::LeftShift },
            //{ "LeftControl", KeyCode::LeftControl },
            //{ "LEFT_ALT", KeyCode::LeftAlt },
            //{ "LEFT_SUPER", KeyCode::LeftSuper },
           // { "RightShift", KeyCode::RightShift },
            //{ "RightControl", KeyCode::RightControl },
            //{ "RIGHT_ALT", KeyCode::RightAlt },
            //{ "RIGHT_SUPER", KeyCode::RightSuper },
            //{ "Menu", KeyCode::Menu },
            { "F1", KeyCode::f1 },
            { "F2", KeyCode::f2 },
            { "F3", KeyCode::f3 },
            { "F4", KeyCode::f4 },
            { "F5", KeyCode::f5 },
            { "F6", KeyCode::f6 },
            { "F7", KeyCode::f7 },
            { "F8", KeyCode::f8 },
            { "F9", KeyCode::f9 },
            { "F10", KeyCode::f10 },
            { "F11", KeyCode::f11 },
            { "F12", KeyCode::f12 },
            { "Keypad0", KeyCode::kp_0 },
            { "Keypad1", KeyCode::kp_1 },
            { "Keypad2", KeyCode::kp_2 },
            { "Keypad3", KeyCode::kp_3 },
            { "Keypad4", KeyCode::kp_4 },
            { "Keypad5", KeyCode::kp_5 },
            { "Keypad6", KeyCode::kp_6 },
            { "Keypad7", KeyCode::kp_7 },
            { "Keypad8", KeyCode::kp_8 },
            { "Keypad9", KeyCode::kp_9 },
            { "Decimal", KeyCode::period },
            { "Divide", KeyCode::slash },
            //{ "Multiply", KeyCode::KPMultiply },
            //{ "Subtract", KeyCode::Minus },
            //{ "Add", KeyCode::KPAdd },
            { "KP_EQUAL", KeyCode::equals }
        };

        state.new_enum<KeyCode, false>("Key", keyItems);

        std::initializer_list<std::pair<sol::string_view, KeyMouse>> mouseItmes = {
                {"Left", KeyMouse::Left},
                {"Right", KeyMouse::Right},
                {"Middle", KeyMouse::Middle},
        };

        state.new_enum<KeyMouse, false>("MouseButton", mouseItmes);
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
        BindInput(*m_State);
    }

    ScriptSystem::~ScriptSystem()
    {
        delete m_State;
    }
}