#pragma once

#include "Asset/Asset.h"
#include <sol/forward.hpp>

namespace MamontEngine
{
	class Script : public Asset
	{
    public:
        explicit Script();
        ~Script();

		virtual void Load(std::string_view filePath) override;

        sol::environment& GetEnvironment()
        {
            return *m_Environment;
        }

    private:
        std::shared_ptr<sol::environment> m_Environment;
	};
}