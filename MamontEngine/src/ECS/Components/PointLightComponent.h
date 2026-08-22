#pragma once

#include "ECS/Components/LightComponent.h"

namespace MamontEngine
{
    struct PointLightComponent : public LightComponent
    {
    public:
        PointLightComponent()      = default;
        ~PointLightComponent() = default;

        inline float GetRadius() const { return m_Radius; }
        inline float GetAttenuation() const
        {
            return m_Attenuation;
        }

    private:
        float m_Radius{1.f};
        float m_Attenuation{1.f};

        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {
            ar(cereal::make_nvp("component", cereal::base_class<LightComponent>(this)));
            ar(cereal::make_nvp("radius", m_Radius));
            ar(cereal::make_nvp("attenuation", m_Attenuation));
        }

        REFLECT();
    };
} // namespace MamontEngine
