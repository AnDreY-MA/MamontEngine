#include "Physics/Collision/SphereCollision.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        IMPLEMENT_REFLECT_COMPONENT(SphereCollision, "BoxCollision Component")
        {
            meta.data<&SphereCollision::m_Radius>(entt::hashed_string{"Radius"}, "Radius");
        }
        FINISH_REFLECT()

    } // namespace HeroPhysics
} // namespace MamontEngine
