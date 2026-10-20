#pragma once

#include "ECS/Components/Component.h"
#include "Math/Color.h"

namespace MamontEngine
{
	struct LightComponent : public Component
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

        float Intensity{5.f};
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