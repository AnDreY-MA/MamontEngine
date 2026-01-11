#pragma once

#include "ECS/Components/LightComponent.h"

namespace MamontEngine
{
    struct DirectionLightComponent : public LightComponent
    {
    public:
        DirectionLightComponent()  = default;
        ~DirectionLightComponent() = default;

    private:
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {
            ar(cereal::make_nvp("component", cereal::base_class<LightComponent>(this)));
        }
    };
}