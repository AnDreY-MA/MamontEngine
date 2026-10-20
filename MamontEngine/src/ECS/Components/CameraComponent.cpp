#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/ComponentRegistry.h"
#include "Utils/Reflection.h"


namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(CameraComponent, "CameraComponent")
    {
        //meta.base<Component>();
        meta.data<&CameraComponent::m_IsViewTarget, entt::as_ref_t>("ViewTarget");

        //ComponentRegistry::Instance().Register<CameraComponent>("Camera Component");
    }
    FINISH_REFLECT()

    CameraComponent::CameraComponent()
    {
        m_Camera = std::make_shared<Camera>();
    }

    CameraComponent::~CameraComponent()
    {

    }
}