#include "ECS/Components/ScriptComponent.h"
#include <sol/sol.hpp>
#include "Core/Engine.h"
#include "Utils/Reflection.h"
#include "Core/Log.h"
#include "Scripting/Script.h"

namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(ScriptComponent, "ScriptComponent")
    {
        meta.base<Component>();
        meta.data<&ScriptComponent::m_Script, entt::as_ref_t>("Script");

    }
    FINISH_REFLECT()

    ScriptComponent::ScriptComponent()
    {
        m_Script = std::make_shared<Script>();
    }

    ScriptComponent::ScriptComponent(std::string_view inFile)
    {
        m_Script = std::make_shared<Script>();
        m_Script->Load(inFile);
    }

    ScriptComponent::~ScriptComponent()
    {

    }

    void ScriptComponent::BeginPlay(Entity inOwner)
    {
        if (!m_OnBeginFunc) return;

        m_EntityOwner = inOwner;

        auto result = m_OnBeginFunc->call();
        if (!result.valid())
        {
            sol::error err = result;
            Log::Error("Failed call BeginPlay function in script\n Error: {}", err.what());
            return;
        }
    }

    void ScriptComponent::Update(float inDeltaTime)
    {
        if (!m_OnUpdateFunc)
            return;

        auto result = m_OnUpdateFunc->call();
        if (!result.valid())
        {
            sol::error err = result;
            Log::Error("Failed call Update function in script\n Error: {}", err.what());
        }
    }

    void ScriptComponent::EndPlay()
    {
        if (!m_OnEndFunc) return;

        auto result = m_OnEndFunc->call();
        if (!result.valid())
        {
            Log::Error("Failed call EndPlay function in script");
        }
    }

    void ScriptComponent::Reload()
    {
        if (!m_Script || !m_Script->IsLoaded())
            return;

        auto &luaState = MEngine::Get().GetScriptSystem()->GetState();

        auto &environment = m_Script->GetEnvironment();

        m_OnBeginFunc = std::make_shared<sol::protected_function>((environment)["BeginPlay"]);
        if (!m_OnBeginFunc->valid())
        {
            m_OnBeginFunc.reset();
        }
        m_OnUpdateFunc = std::make_shared<sol::protected_function>((environment)["Update"]);
        if (!m_OnUpdateFunc->valid())
        {
            m_OnUpdateFunc.reset();
        }
        m_OnEndFunc = std::make_shared<sol::protected_function>((environment)["EndPlay"]);
        if (!m_OnEndFunc->valid())
        {
            m_OnEndFunc.reset();
        }

        (environment)["ScriptComponent"] = this;

        luaState.collect_garbage();

    }
}