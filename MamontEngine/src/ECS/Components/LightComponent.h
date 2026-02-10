#pragma once

#include "Math/Color.h"
#include <cereal/cereal.hpp>
#include "Utils/MetaReflection.h"

namespace MamontEngine
{
	struct LightComponent
	{
    public:
        LightComponent()          = default;

		LightComponent(const LightComponent &other) : 
			Intensity(other.Intensity), Color(other.Color)
		{

		}

		inline glm::vec3 GetColor() const
		{
            return Color.ToVector3() * Intensity;
		}

        float Intensity{1.f};
        Color Color;

	private:
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {
            ar(Intensity, Color);
        }
	};
}