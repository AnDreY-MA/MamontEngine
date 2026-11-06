#include "Scripting/Script.h"
#include <sol/sol.hpp>
#include "Core/Engine.h"
#include "Utils/Reflection.h"
#include "Core/Log.h"

namespace MamontEngine
{
    Script::Script()
    {
        m_Type = EAssetType::Script;
    }

    Script::~Script()
    {

    }

    void Script::Load(std::string_view filePath)
    {
        auto &luaState = MEngine::Get().GetScriptSystem()->GetState();

        m_Environment = std::make_shared<sol::environment>(luaState, sol::create, luaState.globals());

        auto loadResult = luaState.script_file(filePath.data(), *m_Environment, sol::script_pass_on_error);
        if (!loadResult.valid())
        {
            sol::error error = loadResult;
            Log::Error("Failed load script: {}. \nError: {}", filePath.data(), error.what());

            return;
        }



        m_Loaded   = true;
        m_FilePath = filePath.data();
    }
}