#include "ECS/Components/PointLightComponent.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(PointLightComponent, "PointLightComponent")
    {
        meta.base<LightComponent>();
        meta.data<&PointLightComponent::Color>("Color");
        meta.data<&PointLightComponent::Intensity, entt::as_ref_t>("Intensity");
        meta.data<&PointLightComponent::m_Radius, entt::as_ref_t>("Radius");
        meta.data<&PointLightComponent::m_Attenuation, entt::as_ref_t>("Attenuation");
    }
    FINISH_REFLECT()
}