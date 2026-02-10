#include "ECS/Components/RigidbodyComponent.h"
#include "Physics/Body/Rigidbody.h"
#include "Core/Engine.h"
#include "Utils/Reflection.h"
#include "Core/Log.h"

namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(RigidbodyComponent, "RigidbodyComponent")
    {
        meta.data<&RigidbodyComponent::Rigidbody>("Rigidbody");
    }
    FINISH_REFLECT()

    RigidbodyComponent::RigidbodyComponent()
    {
        Rigidbody = MEngine::Get().GetPhysicsSytem()->CreateBody();
    }

    RigidbodyComponent::~RigidbodyComponent()
    {
        MEngine::Get().GetPhysicsSytem()->DestroyBody(Rigidbody); 
    }
} // namespace MamontEngine
