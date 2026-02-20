#pragma once

#include <sol/forward.hpp>

namespace MamontEngine
{
	struct ScriptComponent
	{
    public:
        ScriptComponent() = default;
        ScriptComponent(std::string_view inFile);
        ~ScriptComponent();

        void BeginPlay();
        void Update(float inDeltaTime);
        void EndPlay();

        void        LoadScript(std::string_view inFile);
        inline const std::string& GetFilePath() const
        {
            return FileName;
        }

	private:
        std::shared_ptr<sol::environment>        m_Environment;
        std::shared_ptr<sol::protected_function> m_OnBeginFunc;
        std::shared_ptr<sol::protected_function> m_OnUpdateFunc;
        std::shared_ptr<sol::protected_function> m_OnEndFunc;
        std::string                              FileName{std::string("")};


	};
}