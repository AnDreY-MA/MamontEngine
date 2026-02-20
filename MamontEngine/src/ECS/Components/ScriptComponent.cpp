#include "ECS/Components/ScriptComponent.h"
#include <sol/sol.hpp>
#include "Core/Engine.h"

namespace MamontEngine
{
    ScriptComponent::ScriptComponent(std::string_view inFile)
    {
        LoadScript(inFile);
    }

    ScriptComponent::~ScriptComponent()
    {
        if (m_Environment)
        {
            sol::protected_function releaseFunc = (*m_Environment)["OnRelease"];
            if (releaseFunc.valid())
                releaseFunc.call();
        }
    }

    void ScriptComponent::BeginPlay()
    {
        if (!m_OnBeginFunc) return;

        auto result = m_OnBeginFunc->call();
        if (!result.valid())
        {
            sol::error err = result;
            Log::Error("Failed call BeginPlay function in script\n Error: {}", err.what());
        }
    }


    void ScriptComponent::Update(float inDeltaTime)
    {
        if (!m_OnUpdateFunc)
            return;

        auto result = m_OnUpdateFunc->call();
        if (!result.valid())
        {
            Log::Error("Failed call Update function in script");
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

    void ScriptComponent::LoadScript(std::string_view inFile)
    {
        FileName = inFile;

        auto &luaState = MEngine::Get().GetScriptSystem()->GetState();

        m_Environment = std::make_shared<sol::environment>(luaState, sol::create, luaState.globals());

        auto loadResult = luaState.script_file(FileName, *m_Environment, sol::script_pass_on_error);
        if (!loadResult.valid())
        {
            sol::error error = loadResult;
            Log::Error("Failed load script: {}. \nError: {}", FileName.c_str(), error.what());

            return;
        }

        m_OnBeginFunc = std::make_shared<sol::protected_function>((*m_Environment)["BeginPlay"]);
        if (!m_OnBeginFunc->valid())
        {
            m_OnBeginFunc.reset();
        }
        m_OnUpdateFunc = std::make_shared<sol::protected_function>((*m_Environment)["Update"]);
        if (!m_OnUpdateFunc->valid())
        {
            m_OnUpdateFunc.reset();
        }
        m_OnEndFunc = std::make_shared<sol::protected_function>((*m_Environment)["EndPlay"]);
        if (!m_OnEndFunc->valid())
        {
            m_OnEndFunc.reset();
        }

        (*m_Environment)["ScriptComponent"] = this;

        luaState.collect_garbage();
    }
}