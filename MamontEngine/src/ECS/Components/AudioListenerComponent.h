#pragma once

#include "Component.h"
#include <cereal/cereal.hpp>

namespace MamontEngine
{
	struct AudioListenerComponent : public Component
	{
        AudioListenerComponent() = default;
        ~AudioListenerComponent() = default;
        bool IsEnabled{true};

		REFLECT()

	private:
        friend class cereal::access;

        template <typename Archive>
        void serialize(Archive &ar)
        {
            ar(cereal::make_nvp("Enabled", IsEnabled));
        }
	};
}