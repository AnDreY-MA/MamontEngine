#pragma once

#include "ECS/Components/LightComponent.h"

namespace MamontEngine
{
    struct DirectionLightComponent : public LightComponent
    {
        DirectionLightComponent() = default;
        ~DirectionLightComponent() = default;
    };
}