#include "ECS/Components/TransformComponent.h"
#include "Core/Log.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(TransformComponent, "Transform Component")
    {
        meta.data<&TransformComponent::Transform, entt::as_ref_t>("Transform"_hs);
    }
    FINISH_REFLECT()
}